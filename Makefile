CC := gcc
LD := ld
NASM := nasm
QEMU := qemu-system-x86_64
BUILD_DIR := .build
KERNEL_BIN := $(BUILD_DIR)/imos.elf
SHELL_BIN := $(BUILD_DIR)/shell.elf
CLEAR_BIN := $(BUILD_DIR)/clear.elf
GETTY_BIN := $(BUILD_DIR)/getty.elf
LOGIN_BIN := $(BUILD_DIR)/login.elf
INIT_SCRIPT_OBJ := $(BUILD_DIR)/userspace/init/init_script.o

CFLAGS := -m64 -mno-red-zone -fcf-protection=none -ffreestanding -fno-pie -fno-stack-protector -nostdlib -Wall -Wextra \
	-Iarch/x86_64/include \
	-Idrivers/include \
	-Ikernel/include \
	-Ilib/common/include \
	-Ilib/libc/include \
	-Ilib/libkern/include \
	-Iuserspace/include


NASMFLAGS := -f elf64
LDFLAGS := -m elf_x86_64 -n --gc-sections

C_SRCS := \
	arch/x86_64/src/idt.c \
	arch/x86_64/src/pic.c \
	arch/x86_64/src/port.c \
	drivers/src/print.c \
	drivers/src/ps2.c \
	drivers/src/rtc.c \
	drivers/src/video.c \
	kernel/main.c \
	kernel/src/process.c \
	kernel/src/scheduler.c \
	kernel/src/syscall_dispatch.c \
	kernel/src/input.c \
	kernel/src/paging.c \
	lib/libkern/src/kprintf.c \
	lib/libkern/src/kmalloc.c \
	lib/libkern/src/kassert.c \
	lib/common/src/string.c \
	arch/x86_64/src/syscall_msr.c \
	arch/x86_64/src/tss.c \
	kernel/src/user.c

LIBC_SRCS := \
	lib/libc/src/syscall_wrapper.c \
	lib/libc/src/unistd.c \
	lib/libc/src/stdio.c \
	lib/libc/src/errno.c \
	lib/libc/src/crt0.c

ASM_SRCS := \
	arch/x86_64/boot/header.asm \
	arch/x86_64/boot/main.asm \
	arch/x86_64/boot/main64.asm \
	arch/x86_64/src/idt_.asm \
	arch/x86_64/src/syscall_entry.asm \
	arch/x86_64/src/user_entry.asm \
	arch/x86_64/src/port_.asm

C_OBJS := $(patsubst %.c,$(BUILD_DIR)/%.o,$(C_SRCS))
LIBC_OBJS := $(patsubst %.c,$(BUILD_DIR)/%.o,$(LIBC_SRCS))
ASM_OBJS := $(patsubst %.asm,$(BUILD_DIR)/%.o,$(ASM_SRCS))

.PHONY: all build libc init shell clear getty login iso run qemu clean

all: build
build: $(KERNEL_BIN)

libc: $(LIBC_OBJS)

init: $(BUILD_DIR)/init.elf
shell: $(SHELL_BIN)
clear: $(CLEAR_BIN)
getty: $(GETTY_BIN)
login: $(LOGIN_BIN)

$(BUILD_DIR)/init.elf: $(BUILD_DIR)/userspace/init/init.o $(INIT_SCRIPT_OBJ) $(LIBC_OBJS) $(BUILD_DIR)/lib/common/src/string.o userspace/init/init.ld
	mkdir -p $(dir $@)
	$(LD) $(LDFLAGS) -T userspace/init/init.ld -o $@ $(BUILD_DIR)/lib/libc/src/crt0.o $(BUILD_DIR)/userspace/init/init.o $(INIT_SCRIPT_OBJ) $(BUILD_DIR)/lib/libc/src/stdio.o $(BUILD_DIR)/lib/libc/src/unistd.o $(BUILD_DIR)/lib/libc/src/syscall_wrapper.o $(BUILD_DIR)/lib/libc/src/errno.o $(BUILD_DIR)/lib/common/src/string.o

$(INIT_SCRIPT_OBJ): userspace/init/initfile/initfile.ini

$(SHELL_BIN): $(BUILD_DIR)/userspace/shell/main.o $(BUILD_DIR)/userspace/shell/tty.o $(LIBC_OBJS) $(BUILD_DIR)/lib/common/src/string.o userspace/shell/shell.ld
	mkdir -p $(dir $@)
	$(LD) $(LDFLAGS) -T userspace/shell/shell.ld -o $@ $(BUILD_DIR)/lib/libc/src/crt0.o $(BUILD_DIR)/userspace/shell/main.o $(BUILD_DIR)/userspace/shell/tty.o $(BUILD_DIR)/lib/libc/src/stdio.o $(BUILD_DIR)/lib/libc/src/unistd.o $(BUILD_DIR)/lib/libc/src/syscall_wrapper.o $(BUILD_DIR)/lib/libc/src/errno.o $(BUILD_DIR)/lib/common/src/string.o

$(CLEAR_BIN): $(BUILD_DIR)/userspace/utilities/clear.o $(LIBC_OBJS) userspace/utilities/utility.ld
	mkdir -p $(dir $@)
	$(LD) $(LDFLAGS) -T userspace/utilities/utility.ld -o $@ $(BUILD_DIR)/lib/libc/src/crt0.o $(BUILD_DIR)/userspace/utilities/clear.o $(BUILD_DIR)/lib/libc/src/unistd.o $(BUILD_DIR)/lib/libc/src/syscall_wrapper.o $(BUILD_DIR)/lib/libc/src/errno.o

$(GETTY_BIN): $(BUILD_DIR)/userspace/utilities/getty.o $(LIBC_OBJS) userspace/utilities/utility.ld
	mkdir -p $(dir $@)
	$(LD) $(LDFLAGS) -T userspace/utilities/utility.ld -o $@ $(BUILD_DIR)/lib/libc/src/crt0.o $(BUILD_DIR)/userspace/utilities/getty.o $(BUILD_DIR)/lib/libc/src/stdio.o $(BUILD_DIR)/lib/libc/src/unistd.o $(BUILD_DIR)/lib/libc/src/syscall_wrapper.o $(BUILD_DIR)/lib/libc/src/errno.o $(BUILD_DIR)/lib/common/src/string.o

$(LOGIN_BIN): $(BUILD_DIR)/userspace/utilities/login.o $(LIBC_OBJS) userspace/utilities/utility.ld
	mkdir -p $(dir $@)
	$(LD) $(LDFLAGS) -T userspace/utilities/utility.ld -o $@ $(BUILD_DIR)/lib/libc/src/crt0.o $(BUILD_DIR)/userspace/utilities/login.o $(BUILD_DIR)/lib/libc/src/stdio.o $(BUILD_DIR)/lib/libc/src/unistd.o $(BUILD_DIR)/lib/libc/src/syscall_wrapper.o $(BUILD_DIR)/lib/libc/src/errno.o $(BUILD_DIR)/lib/common/src/string.o

$(KERNEL_BIN): $(C_OBJS) $(ASM_OBJS) arch/x86_64/boot/linker.ld
	mkdir -p $(dir $@)
	$(LD) $(LDFLAGS) -T arch/x86_64/boot/linker.ld -o $@ $(ASM_OBJS) $(C_OBJS)

$(BUILD_DIR)/%.o: %.c
	mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/%.o: %.asm
	mkdir -p $(dir $@)
	$(NASM) $(NASMFLAGS) $< -o $@

iso: $(KERNEL_BIN) $(BUILD_DIR)/init.elf $(SHELL_BIN) $(CLEAR_BIN) $(GETTY_BIN) $(LOGIN_BIN)
	mkdir -p distro/iso/boot/grub
	cp $(KERNEL_BIN) distro/iso/boot/imos.elf
	cp $(BUILD_DIR)/init.elf distro/iso/boot/init.elf
	cp $(SHELL_BIN) distro/iso/boot/shell.elf
	cp $(CLEAR_BIN) distro/iso/boot/clear.elf
	cp $(GETTY_BIN) distro/iso/boot/getty.elf
	cp $(LOGIN_BIN) distro/iso/boot/login.elf
	printf '%s\n' 'set timeout=0' 'set default=0' 'menuentry "Imagine R1" {' '    multiboot2 /boot/imos.elf' '    module2 /boot/init.elf init.elf' '    module2 /boot/shell.elf shell.elf' '    module2 /boot/clear.elf clear.elf' '    module2 /boot/getty.elf getty.elf' '    module2 /boot/login.elf login.elf' '    boot' '}' > distro/iso/boot/grub/grub.cfg
	grub-mkrescue -o distro/coreimage_imagine-astrid.iso distro/iso >/dev/null 2>&1

run: iso
	$(QEMU) -no-reboot -boot d -serial stdio -cdrom distro/coreimage_imagine-astrid.iso

qemu: run

clean:
	rm -rf $(BUILD_DIR) distro/iso distro/coreimage_imagine-astrid.iso distro/

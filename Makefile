CC := gcc
LD := ld
NASM := nasm
QEMU := qemu-system-x86_64
BUILD_DIR := .build
KERNEL_BIN := $(BUILD_DIR)/imos.elf

CFLAGS := -m64 -ffreestanding -fno-pie -fno-stack-protector -nostdlib -Wall -Wextra \
	-Ikernel/include -Idevices/include -Iarch/x86_64/include -Ilib/include -Ilib
NASMFLAGS := -f elf64
LDFLAGS := -m elf_x86_64 -n --gc-sections

C_SRCS := \
	init/init.c \
	arch/x86_64/src/idt.c \
	arch/x86_64/src/pic.c \
	arch/x86_64/src/port.c \
	devices/drivers/keyboard.c \
	devices/drivers/keyboard_keys.c \
	devices/drivers/print.c \
	devices/drivers/ps2.c \
	devices/drivers/rtc.c \
	devices/drivers/video.c \
	kernel/main.c \
	kernel/management/modules.c \
	kernel/management/process.c \
	kernel/management/scheduler.c \
	userspace/common/quackshell/shell.c \
	userspace/common/quackshell/tty.c \
	userspace/common/system/halt.c \
	userspace/common/system/login.c \
	userspace/common/system/login_prompt.c \
	userspace/common/system/string.c \
	userspace/utilities/calc.c \
	userspace/utilities/clear.c \
	userspace/utilities/help.c \
	userspace/utilities/lite.c \
	userspace/utilities/ps.c \
	userspace/utilities/version.c

ASM_SRCS := \
	arch/x86_64/boot/header.asm \
	arch/x86_64/boot/main.asm \
	arch/x86_64/boot/main64.asm \
	arch/x86_64/src/idt_.asm \
	arch/x86_64/src/port_.asm

C_OBJS := $(patsubst %.c,$(BUILD_DIR)/%.o,$(C_SRCS))
ASM_OBJS := $(patsubst %.asm,$(BUILD_DIR)/%.o,$(ASM_SRCS))

.PHONY: all build iso run qemu clean

all: build
build: $(KERNEL_BIN)

$(KERNEL_BIN): $(C_OBJS) $(ASM_OBJS) arch/x86_64/boot/linker.ld
	mkdir -p $(dir $@)
	$(LD) $(LDFLAGS) -T arch/x86_64/boot/linker.ld -o $@ $(ASM_OBJS) $(C_OBJS)

$(BUILD_DIR)/%.o: %.c
	mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/%.o: %.asm
	mkdir -p $(dir $@)
	$(NASM) $(NASMFLAGS) $< -o $@

iso: $(KERNEL_BIN)
	mkdir -p distro/iso/boot/grub
	cp $(KERNEL_BIN) distro/iso/boot/imos.elf
	printf '%s\n' 'set timeout=0' 'set default=0' 'menuentry "ImagineOS" {' '    multiboot2 /boot/imos.elf' '    boot' '}' > distro/iso/boot/grub/grub.cfg
	grub-mkrescue -o distro/imos.iso distro/iso >/dev/null 2>&1

run: iso
	$(QEMU) -no-reboot -serial stdio -cdrom distro/imos.iso

qemu: run

clean:
	rm -rf $(BUILD_DIR) distro/iso distro/imos.iso distro/

CC := gcc
LD := ld
NASM := nasm
QEMU := qemu-system-x86_64
CFLAGS := -m64 -ffreestanding -fno-pie -fno-stack-protector -nostdlib -Wall -Wextra -Iinclude -Iinclude/devices -Iinclude/libraries -Iinclude/management -Iinclude/userspace
NASMFLAGS := -f elf64
LDFLAGS := -m elf_x86_64 -n --gc-sections
BUILD_DIR := .build
KERNEL_BIN := $(BUILD_DIR)/imos.elf

KERNEL_SRCS := \
	main/kernel/main.c \
	main/devices/idt.c \
	main/devices/keyboard.c \
	main/devices/keyboard_keys.c \
	main/devices/pic.c \
	main/devices/port.c \
	main/devices/print.c \
	main/devices/ps2.c \
	main/devices/rtc.c \
	main/management/modules.c \
	main/management/init.c \
	main/management/process.c \
	main/management/scheduler.c \
	userspace/calc.c \
	userspace/clear.c \
	userspace/halt.c \
	userspace/help.c \
	userspace/version.c \
	userspace/video.c \
	userspace/string_compat.c \
	userspace/lite.c \
	userspace/ps.c \
	userspace/QBshell/shell.c \
	userspace/login.c \
	userspace/login_prompt.c \
	userspace/QBshell/tty.c

BOOT_SRCS := \
	main/boot/quin-headers/header.asm \
	main/boot/quin-headers/main.asm \
	main/boot/quin-headers/main64.asm

KERNEL_OBJS := \
	$(BUILD_DIR)/main/kernel/main.o \
	$(BUILD_DIR)/main/devices/idt.o \
	$(BUILD_DIR)/main/devices/keyboard.o \
	$(BUILD_DIR)/main/devices/keyboard_keys.o \
	$(BUILD_DIR)/main/devices/pic.o \
	$(BUILD_DIR)/main/devices/port.o \
	$(BUILD_DIR)/main/devices/print.o \
	$(BUILD_DIR)/main/devices/ps2.o \
	$(BUILD_DIR)/main/devices/rtc.o \
	$(BUILD_DIR)/main/management/modules.o \
	$(BUILD_DIR)/main/management/init.o \
	$(BUILD_DIR)/main/management/process.o \
	$(BUILD_DIR)/main/management/scheduler.o \
	$(BUILD_DIR)/userspace/calc.o \
	$(BUILD_DIR)/userspace/clear.o \
	$(BUILD_DIR)/userspace/halt.o \
	$(BUILD_DIR)/userspace/help.o \
	$(BUILD_DIR)/userspace/version.o \
	$(BUILD_DIR)/userspace/video.o \
	$(BUILD_DIR)/userspace/string_compat.o \
	$(BUILD_DIR)/userspace/lite.o \
	$(BUILD_DIR)/userspace/ps.o \
	$(BUILD_DIR)/userspace/QBshell/shell.o \
	$(BUILD_DIR)/userspace/login.o \
	$(BUILD_DIR)/userspace/login_prompt.o \
	$(BUILD_DIR)/userspace/QBshell/tty.o

BOOT_OBJS := \
	$(BUILD_DIR)/main/boot/quin-headers/header.o \
	$(BUILD_DIR)/main/boot/quin-headers/main.o \
	$(BUILD_DIR)/main/boot/quin-headers/main64.o \
	$(BUILD_DIR)/main/devices/idt_.o \
	$(BUILD_DIR)/main/devices/port_.o

.PHONY: all build iso run clean qemu

all: build

build: $(KERNEL_BIN)

iso: $(KERNEL_BIN)
	mkdir -p distro/iso/boot/grub
	cp $(KERNEL_BIN) distro/iso/boot/imos.elf
	printf '%s\n' 'set timeout=0' 'set default=0' 'menuentry "ImagineOS" {' '    multiboot2 /boot/imos.elf' '    boot' '}' > distro/iso/boot/grub/grub.cfg
	grub-mkrescue -o distro/imos.iso distro/iso >/dev/null 2>&1

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(BUILD_DIR)/main/kernel/main.o: main/kernel/main.c | $(BUILD_DIR)
	mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/main/devices/idt.o: main/devices/idt.c | $(BUILD_DIR)
	mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/main/devices/keyboard.o: main/devices/keyboard.c | $(BUILD_DIR)
	mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/main/devices/keyboard_keys.o: main/devices/keyboard_keys.c | $(BUILD_DIR)
	mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/main/devices/pic.o: main/devices/pic.c | $(BUILD_DIR)
	mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/main/devices/port.o: main/devices/port.c | $(BUILD_DIR)
	mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/main/devices/print.o: main/devices/print.c | $(BUILD_DIR)
	mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/main/devices/ps2.o: main/devices/ps2.c | $(BUILD_DIR)
	mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/main/devices/rtc.o: main/devices/rtc.c | $(BUILD_DIR)
	mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/main/management/modules.o: main/management/modules.c | $(BUILD_DIR)
	mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/main/management/init.o: main/management/init.c | $(BUILD_DIR)
	mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/main/management/process.o: main/management/process.c | $(BUILD_DIR)
	mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/main/management/scheduler.o: main/management/scheduler.c | $(BUILD_DIR)
	mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/userspace/calc.o: userspace/calc.c | $(BUILD_DIR)
	mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/userspace/clear.o: userspace/clear.c | $(BUILD_DIR)
	mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/userspace/halt.o: userspace/halt.c | $(BUILD_DIR)
	mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/userspace/help.o: userspace/help.c | $(BUILD_DIR)
	mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/userspace/version.o: userspace/version.c | $(BUILD_DIR)
	mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/userspace/video.o: userspace/video.c | $(BUILD_DIR)
	mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/userspace/string_compat.o: userspace/string_compat.c | $(BUILD_DIR)
	mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/userspace/lite.o: userspace/lite.c | $(BUILD_DIR)
	mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/userspace/ps.o: userspace/ps.c | $(BUILD_DIR)
	mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/userspace/QBshell/shell.o: userspace/QBshell/shell.c | $(BUILD_DIR)
	mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/userspace/login.o: userspace/login.c | $(BUILD_DIR)
	mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/userspace/login_prompt.o: userspace/login_prompt.c | $(BUILD_DIR)
	mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/userspace/QBshell/tty.o: userspace/QBshell/tty.c | $(BUILD_DIR)
	mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/main/boot/quin-headers/header.o: main/boot/quin-headers/header.asm | $(BUILD_DIR)
	mkdir -p $(dir $@)
	$(NASM) $(NASMFLAGS) $< -o $@

$(BUILD_DIR)/main/boot/quin-headers/main.o: main/boot/quin-headers/main.asm | $(BUILD_DIR)
	mkdir -p $(dir $@)
	$(NASM) $(NASMFLAGS) $< -o $@

$(BUILD_DIR)/main/boot/quin-headers/main64.o: main/boot/quin-headers/main64.asm | $(BUILD_DIR)
	mkdir -p $(dir $@)
	$(NASM) $(NASMFLAGS) $< -o $@

$(BUILD_DIR)/main/devices/idt_.o: main/devices/idt_.asm | $(BUILD_DIR)
	mkdir -p $(dir $@)
	$(NASM) $(NASMFLAGS) $< -o $@

$(BUILD_DIR)/main/devices/port_.o: main/devices/port_.asm | $(BUILD_DIR)
	mkdir -p $(dir $@)
	$(NASM) $(NASMFLAGS) $< -o $@

$(KERNEL_BIN): $(KERNEL_OBJS) $(BOOT_OBJS) main/boot/linker.ld | $(BUILD_DIR)
	$(LD) $(LDFLAGS) -T main/boot/linker.ld -o $@ $(BOOT_OBJS) $(KERNEL_OBJS)

run: iso
	$(QEMU) -no-reboot -serial stdio -cdrom distro/imos.iso

qemu: run

clean:
	rm -rf $(BUILD_DIR) distro/iso distro/imos.iso


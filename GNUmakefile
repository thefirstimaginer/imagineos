# ------------------------------------------------------------------
# Global configuration
# ------------------------------------------------------------------

.SUFFIXES:
.DELETE_ON_ERROR:

override OUTPUT := dreamcore

BUILD_DIR := .build
ISO_DIR := distro/iso
ISO_IMAGE := distro/dreamcore.iso

QEMU := qemu-system-x86_64

# ------------------------------------------------------------------
# Toolchain
# ------------------------------------------------------------------

TOOLCHAIN :=
TOOLCHAIN_PREFIX :=

ifneq ($(TOOLCHAIN),)
    ifeq ($(TOOLCHAIN_PREFIX),)
        TOOLCHAIN_PREFIX := $(TOOLCHAIN)-
    endif
endif

CC := $(if $(TOOLCHAIN_PREFIX),$(TOOLCHAIN_PREFIX)gcc,gcc)
LD := $(TOOLCHAIN_PREFIX)ld
NASM := nasm


ifeq ($(TOOLCHAIN),llvm)
    CC := clang
    LD := ld.lld
endif

# ------------------------------------------------------------------
# Output binaries
# ------------------------------------------------------------------

KERNEL_BIN := $(BUILD_DIR)/dreamcore.elf

# ------------------------------------------------------------------
# Compiler flags
# ------------------------------------------------------------------

override CFLAGS += \
    -g \
    -O2 \
    -Wall \
    -Wextra \
    -std=gnu11 \
    -ffreestanding \
    -fno-stack-protector \
    -fno-stack-check \
    -fno-lto \
    -fno-pie \
    -fno-PIC \
    -ffunction-sections \
    -fdata-sections \
    -m64 \
    -march=x86-64 \
    -mabi=sysv \
    -mno-red-zone \
    -mcmodel=kernel \
    -fcf-protection=none

override CPPFLAGS += \
    -MMD \
    -MP \
    -Iarch/x86_64/include \
    -Idrivers/include \
    -Ikernel/include \
    -Ilib/common/include \
    -Ilib/libc/include \
    -Ilib/libkern/include

override NASMFLAGS += \
    -f elf64 \
    -Wall

override LDFLAGS += \
    -m elf_x86_64 \
    -nostdlib \
    -static \
    --gc-sections

# ------------------------------------------------------------------
# Automatic source discovery
# ------------------------------------------------------------------

override SRCFILES := $(shell find arch drivers kernel lib -type f | sort)

override CFILES := $(filter %.c,$(SRCFILES))
override ASMFILES := $(filter %.asm,$(SRCFILES))

override C_OBJS := $(addprefix $(BUILD_DIR)/,$(CFILES:.c=.o))
override ASM_OBJS := $(addprefix $(BUILD_DIR)/,$(ASMFILES:.asm=.o))

override HEADER_DEPS := $(C_OBJS:.o=.d)

# ------------------------------------------------------------------
# Kernel object selection
# ------------------------------------------------------------------

KERNEL_C_OBJS := \
    $(BUILD_DIR)/arch/x86_64/src/idt.o \
    $(BUILD_DIR)/arch/x86_64/src/pic.o \
    $(BUILD_DIR)/arch/x86_64/src/port.o \
    $(BUILD_DIR)/drivers/src/rtc.o \
    $(BUILD_DIR)/kernel/kernel/main.o \
	$(BUILD_DIR)/kernel/kernel/start.o \
    $(BUILD_DIR)/kernel/src/process.o \
    $(BUILD_DIR)/kernel/src/scheduler.o \
    $(BUILD_DIR)/kernel/src/syscall_dispatch.o \
    $(BUILD_DIR)/kernel/src/input.o \
    $(BUILD_DIR)/kernel/src/paging.o \
    $(BUILD_DIR)/kernel/src/user.o \
	$(BUILD_DIR)/kernel/src/hcf.o \
    $(BUILD_DIR)/lib/libkern/src/kprintf.o \
    $(BUILD_DIR)/lib/libkern/src/kmalloc.o \
    $(BUILD_DIR)/lib/libkern/src/kassert.o \
    $(BUILD_DIR)/lib/common/src/string.o \
    $(BUILD_DIR)/arch/x86_64/src/syscall_msr.o \
    $(BUILD_DIR)/arch/x86_64/src/tss.o \
	$(BUILD_DIR)/arch/x86_64/src/gdt.o

KERNEL_ASM_OBJS := \
    $(BUILD_DIR)/arch/x86_64/src/idt_.o \
    $(BUILD_DIR)/arch/x86_64/src/port_.o \
    $(BUILD_DIR)/arch/x86_64/src/syscall_entry.o \
    $(BUILD_DIR)/arch/x86_64/src/user_entry.o

# ------------------------------------------------------------------
# Main targets
# ------------------------------------------------------------------

.PHONY: all
all: $(KERNEL_BIN)

-include $(HEADER_DEPS)

$(KERNEL_BIN): $(KERNEL_C_OBJS) $(KERNEL_ASM_OBJS) arch/x86_64/boot/linker.lds
	mkdir -p $(dir $@)
	$(LD) $(LDFLAGS) \
	-T arch/x86_64/boot/linker.lds \
	-o $@ \
	$(KERNEL_ASM_OBJS) \
	$(KERNEL_C_OBJS)

# ------------------------------------------------------------------
# Generic rules
# ------------------------------------------------------------------

$(BUILD_DIR)/%.o: %.c
	mkdir -p $(dir $@)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/%.o: %.asm
	mkdir -p $(dir $@)
	$(NASM) $(NASMFLAGS) $< -o $@

# ------------------------------------------------------------------
# Limine ISO
# ------------------------------------------------------------------

.PHONY: iso

iso: $(KERNEL_BIN)

	mkdir -p distro/iso/boot
	mkdir -p distro/iso/boot/limine
	mkdir -p distro/iso/EFI/BOOT

	cp $(KERNEL_BIN) distro/iso/boot/dreamcore.elf

	cp arch/x86_64/boot/limine.conf distro/iso/boot/limine/

	cp toolchain/limine-binary/limine-bios-cd.bin distro/iso/boot/limine/
	cp toolchain/limine-binary/limine-bios.sys    distro/iso/boot/limine/
	cp toolchain/limine-binary/limine-uefi-cd.bin distro/iso/boot/limine/

	cp toolchain/limine-binary/BOOTX64.EFI distro/iso/EFI/BOOT/
	cp toolchain/limine-binary/BOOTIA32.EFI distro/iso/EFI/BOOT/

	xorriso -as mkisofs \
	    -b boot/limine/limine-bios-cd.bin \
	    -no-emul-boot \
	    -boot-load-size 4 \
	    -boot-info-table \
	    --efi-boot EFI/BOOT/BOOTX64.EFI \
	    -efi-boot-part \
	    --efi-boot-image \
	    --protective-msdos-label \
	    distro/iso \
	    -o distro/dreamcore.iso	

	toolchain/limine-binary/limine bios-install $(ISO_IMAGE)

# ------------------------------------------------------------------
# QEMU
# ------------------------------------------------------------------

.PHONY: run

run: iso
	$(QEMU) \
	    -M q35 \
	    -m 512M \
	    -serial stdio \
	    -cdrom $(ISO_IMAGE)

# ------------------------------------------------------------------
# Clean
# ------------------------------------------------------------------

.PHONY: clean

clean:
	rm -rf $(BUILD_DIR)
	rm -rf distro
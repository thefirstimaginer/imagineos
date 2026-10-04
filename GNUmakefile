SHELL := /bin/sh

KERNEL_NAME := dreamcore
CODENAME := astrid

TARGET := x86_64-unknown-none
KERNEL_FEATURES ?=
KERNEL := target/$(TARGET)/release/dreamcore
ISO_DIR := .build/iso
RAMFS_IMAGE := .build/ramfs.tar
USER_UTILITIES := cat grep ls
USER_PROGRAMS := .build/user/init.elf .build/user/getty.elf .build/user/shell.elf \
	$(addprefix .build/user/utilities/,$(addsuffix .elf,$(USER_UTILITIES)))
RAMFS_FILES := $(shell find ramfs -type f | sort)
ISO_IMAGE := distro/$(KERNEL_NAME)-$(shell date +%Y-%m-%d-%H-%M)-$(CODENAME).iso
QEMU := qemu-system-x86_64
OVMF_CODE ?= /usr/share/OVMF/OVMF_CODE.fd

.PHONY: all kernel user-programs iso run clean

all: kernel

kernel: linker.ld
	rustup run stable cargo build --release --target $(TARGET) $(if $(KERNEL_FEATURES),--features $(KERNEL_FEATURES),)

$(RAMFS_IMAGE): $(USER_PROGRAMS) $(RAMFS_FILES)
	rm -rf .build/ramfs
	mkdir -p .build/ramfs
	cp -a ramfs/. .build/ramfs/
	cp .build/user/init.elf .build/ramfs/bin/init
	cp .build/user/getty.elf .build/ramfs/bin/getty
	cp .build/user/shell.elf .build/ramfs/bin/shell
	for utility in $(USER_UTILITIES); do cp .build/user/utilities/$$utility.elf .build/ramfs/bin/$$utility; done
	tar --format=ustar -C .build/ramfs -cf $@ bin home system tmp usr

.PHONY: user-programs
user-programs: $(USER_PROGRAMS)

.build/user/%.elf: userspace/%.rs userspace/linker.ld
	mkdir -p .build/user
	rustup run stable rustc --crate-name $* --edition 2021 --target $(TARGET) \
		-C panic=abort -C relocation-model=static \
		-C link-arg=-Tuserspace/linker.ld $< -o $@

.build/user/utilities/%.elf: userspace/utilities/%.rs userspace/utilities/common.rs userspace/linker.ld
	mkdir -p .build/user/utilities
	rustup run stable rustc --crate-name $* --edition 2021 --target $(TARGET) \
		-C panic=abort -C relocation-model=static \
		-C link-arg=-Tuserspace/linker.ld $< -o $@

iso: kernel $(RAMFS_IMAGE)
	command -v xorriso >/dev/null
	command -v mkfs.vfat >/dev/null
	command -v mcopy >/dev/null
	command -v mmd >/dev/null
	rm -rf $(ISO_DIR)
	mkdir -p $(ISO_DIR)/EFI/BOOT $(ISO_DIR)/boot distro
	cp $(KERNEL) $(ISO_DIR)/boot/kernel.elf
	cp $(RAMFS_IMAGE) $(ISO_DIR)/boot/ramfs.tar
	cp limine.conf $(ISO_DIR)/limine.conf
	cp toolchain/limine-binary/BOOTX64.EFI $(ISO_DIR)/EFI/BOOT/BOOTX64.EFI
	dd if=/dev/zero of=$(ISO_DIR)/efi.img bs=1M count=16
	mkfs.vfat $(ISO_DIR)/efi.img
	mmd -i $(ISO_DIR)/efi.img ::/EFI ::/EFI/BOOT ::/boot
	mcopy -i $(ISO_DIR)/efi.img $(ISO_DIR)/EFI/BOOT/BOOTX64.EFI ::/EFI/BOOT/
	mcopy -i $(ISO_DIR)/efi.img $(ISO_DIR)/limine.conf ::/limine.conf
	mcopy -i $(ISO_DIR)/efi.img $(ISO_DIR)/boot/kernel.elf ::/boot/kernel.elf
	mcopy -i $(ISO_DIR)/efi.img $(ISO_DIR)/boot/ramfs.tar ::/boot/ramfs.tar
	xorriso -as mkisofs -R -r -J -V DREAMCORE \
		--efi-boot efi.img -efi-boot-part --efi-boot-image \
		--protective-msdos-label $(ISO_DIR) -o $(ISO_IMAGE)
	@printf 'ISO gerada: %s\n' '$(ISO_IMAGE)'

run: iso
	test -f "$(OVMF_CODE)"
	$(QEMU) -machine q35 -m 512M -serial stdio -bios "$(OVMF_CODE)" -cdrom $(ISO_IMAGE)

clean:
	rm -rf target .build
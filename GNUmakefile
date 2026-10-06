SHELL := /bin/sh

KERNEL_NAME := dreamcore
CODENAME := astrid

TARGET := x86_64-unknown-none
KERNEL_FEATURES ?=
KERNEL := target/$(TARGET)/release/dreamcore
ISO_DIR := .build/iso
RAMFS_IMAGE := .build/ramfs.tar
USER_UTILITIES := cat globalconf grep ls mkdir rm touch vi hello
RAMFS_DIRS := bin sbin home system/fonts tmp usr
USER_API_MANIFEST := userland/api/Cargo.toml
USER_API_TARGET_DIR := .build/user/api-target
USER_API_RLIB := $(USER_API_TARGET_DIR)/$(TARGET)/release/libimagineos.rlib
USER_API_DEPS := $(USER_API_TARGET_DIR)/$(TARGET)/release/deps
USER_PROGRAMS := .build/user/sbin/init .build/user/sbin/getty \
	.build/user/bin/shell \
	$(addprefix .build/user/utilities/,$(USER_UTILITIES))
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
	mkdir -p $(addprefix .build/ramfs/,$(RAMFS_DIRS))
	cp -a ramfs/. .build/ramfs/
	cp .build/user/sbin/init .build/ramfs/sbin/init
	cp .build/user/sbin/getty .build/ramfs/sbin/getty
	cp .build/user/bin/shell .build/ramfs/bin/shell
	for utility in $(USER_UTILITIES); do cp .build/user/utilities/$$utility .build/ramfs/bin/$$utility; done
	tar --format=ustar -C .build/ramfs -cf $@ bin sbin home system tmp usr

.PHONY: user-programs
user-programs: $(USER_PROGRAMS)

.build/user/sbin/%: userland/%.rs userland/linker.ld $(USER_API_RLIB)
	mkdir -p $(dir $@)
	rustup run stable rustc --crate-name $* --edition 2021 --target $(TARGET) \
		--extern imagineos=$(USER_API_RLIB) -L dependency=$(USER_API_DEPS) \
		-C panic=abort -C relocation-model=static \
		-C link-arg=-Tuserland/linker.ld $< -o $@

.build/user/bin/%: userland/%.rs userland/linker.ld $(USER_API_RLIB)
	mkdir -p $(dir $@)
	rustup run stable rustc --crate-name $* --edition 2021 --target $(TARGET) \
		--extern imagineos=$(USER_API_RLIB) -L dependency=$(USER_API_DEPS) \
		-C panic=abort -C relocation-model=static \
		-C link-arg=-Tuserland/linker.ld $< -o $@

.build/user/utilities/%: userland/utilities/%.rs userland/utilities/common.rs userland/linker.ld $(USER_API_RLIB)
	mkdir -p $(dir $@)
	rustup run stable rustc --crate-name $* --edition 2021 --target $(TARGET) \
		--extern imagineos=$(USER_API_RLIB) -L dependency=$(USER_API_DEPS) \
		-C panic=abort -C relocation-model=static \
		-C link-arg=-Tuserland/linker.ld $< -o $@

$(USER_API_RLIB): $(USER_API_MANIFEST) userland/api/src/lib.rs \
	userland/api/src/args.rs userland/api/src/console.rs userland/api/src/fs.rs \
	userland/api/src/legacy.rs userland/api/src/process.rs userland/api/src/syscall.rs \
	shared/abi/Cargo.toml shared/abi/src/lib.rs
	rustup run stable cargo build --manifest-path $(USER_API_MANIFEST) \
		--target $(TARGET) --release --target-dir $(USER_API_TARGET_DIR)

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
	$(QEMU) -machine q35 -m 512M -serial stdio \
		-drive if=pflash,format=raw,unit=0,readonly=on,file="$(OVMF_CODE)" \
		-cdrom $(ISO_IMAGE)

clean:
	rm -rf target .build
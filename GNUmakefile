SHELL := /bin/sh

KERNEL_NAME := dreamcore
CODENAME := astrid

TARGET := x86_64-unknown-none
KERNEL_FEATURES ?=
KERNEL := target/$(TARGET)/release/dreamcore
BOOTSTRAP := .build/bootstrap.elf
DZ_IMAGE := .build/dzImage
ISO_DIR := .build/iso
RAMFS_IMAGE := .build/ramfs.tar
RAMFS_INSTALLED_IMAGE := .build/ramfs-installed.tar
USER_UTILITIES := cat dmesg distroinstall fdtest globalconf grep kill ls mkdir ps rm shutdown su sudo touch uname vi hello
RAMFS_DIRS := bin dev sbin home system/fonts system/install tmp usr
USER_API_MANIFEST := userland/api/Cargo.toml
USER_API_TARGET_DIR := .build/user/api-target
USER_API_RLIB := $(USER_API_TARGET_DIR)/$(TARGET)/release/libimagineos.rlib
USER_API_DEPS := $(USER_API_TARGET_DIR)/$(TARGET)/release/deps
USER_RUNTIME_MANIFEST := userland/runtime/Cargo.toml
USER_RUNTIME_TARGET_DIR := .build/user/runtime-target
USER_RUNTIME_RLIB := $(USER_RUNTIME_TARGET_DIR)/$(TARGET)/release/libimagineos_rt.rlib
USER_RUNTIME_DEPS := $(USER_RUNTIME_TARGET_DIR)/$(TARGET)/release/deps
USER_PROGRAMS := .build/user/sbin/init .build/user/sbin/getty \
	.build/user/bin/shell \
	$(addprefix .build/user/utilities/,$(USER_UTILITIES))
RAMFS_FILES := $(shell find ramfs -type f | sort)
ISO_IMAGE := distro/$(KERNEL_NAME)-$(shell date +%Y-%m-%d-%H-%M)-$(CODENAME).iso
DISK_IMAGE := .build/imagineos-disk.img
INSTALL_TARGET_IMAGE := .build/installer-target.img
QEMU := qemu-system-x86_64
OVMF_CODE ?= /usr/share/OVMF/OVMF_CODE.fd

.PHONY: all kernel bootstrap dzimage user-programs iso run disk-image run-disk installer-disk run-installer clean

all: kernel

kernel: linker.ld tools/fonts/zap-vga16.psf
	rustup run stable cargo build --release --target $(TARGET) --bin dreamcore \
		--config 'target.x86_64-unknown-none.rustflags=["-C","link-arg=-Tlinker.ld","-C","relocation-model=static"]' \
		$(if $(KERNEL_FEATURES),--features $(KERNEL_FEATURES),)

bootstrap: $(BOOTSTRAP)

$(BOOTSTRAP): dnu/bootstrap.rs dnu/boot_info.rs dnu/dzimage.rs dnu/time.rs \
	dnu/console/framebuffer.rs tools/fonts/zap-vga16.psf bootstrap.ld
	rustup run stable cargo build --release --target $(TARGET) --bin bootstrap \
		--features bootstrap \
		--target-dir .build/bootstrap-target \
		--config 'target.x86_64-unknown-none.rustflags=["-C","link-arg=-Tbootstrap.ld","-C","relocation-model=static"]'
	mkdir -p .build
	cp .build/bootstrap-target/$(TARGET)/release/bootstrap $@

.build/dzpack: tools/dzpack.rs dnu/dzimage.rs
	mkdir -p .build
	rustup run stable rustc --edition 2021 $< -o $@

.build/dfs-image: tools/dfs-image.rs dnu/fs/dfs.rs dnu/fs/gpt.rs dnu/drivers/block.rs
	mkdir -p .build
	rustup run stable rustc --edition 2021 $< -o $@

dzimage: $(DZ_IMAGE)

$(DZ_IMAGE): $(KERNEL) .build/dzpack
	.build/dzpack $(KERNEL) $@

$(RAMFS_IMAGE): kernel $(BOOTSTRAP) $(DZ_IMAGE) $(USER_PROGRAMS) $(RAMFS_FILES) tools/fonts/zap-vga16.psf toolchain/limine-binary/BOOTX64.EFI limine.conf tools/startup.nsh
	rm -rf .build/ramfs
	mkdir -p $(addprefix .build/ramfs/,$(RAMFS_DIRS))
	cp -a ramfs/. .build/ramfs/
	if [ ! -e .build/ramfs/system/fonts/zap-vga16.psf ]; then \
		cp tools/fonts/zap-vga16.psf .build/ramfs/system/fonts/zap-vga16.psf; \
	fi
	cp .build/user/sbin/init .build/ramfs/sbin/init
	cp .build/user/sbin/getty .build/ramfs/sbin/getty
	cp .build/user/bin/shell .build/ramfs/bin/shell
	for utility in $(USER_UTILITIES); do cp .build/user/utilities/$$utility .build/ramfs/bin/$$utility; done
	tar --format=ustar --numeric-owner --owner=0 --group=0 \
		--exclude=bin/distroinstall --exclude='system/install/*' \
		-C .build/ramfs -cf $(RAMFS_INSTALLED_IMAGE) bin dev sbin home system tmp usr
	cp $(BOOTSTRAP) .build/ramfs/system/install/bootstrap.elf
	cp $(DZ_IMAGE) .build/ramfs/system/install/dzImage
	cp toolchain/limine-binary/BOOTX64.EFI .build/ramfs/system/install/BOOTX64.EFI
	cp limine.conf .build/ramfs/system/install/limine.conf
	cp tools/startup.nsh .build/ramfs/system/install/startup.nsh
	cp $(RAMFS_INSTALLED_IMAGE) .build/ramfs/system/install/ramfs-installed.tar
	tar --format=ustar --numeric-owner --owner=0 --group=0 \
		-C .build/ramfs -cf $@ bin dev sbin home system tmp usr

.PHONY: user-programs
user-programs: $(USER_PROGRAMS)

.build/user/sbin/%: userland/%.rs userland/linker.ld $(USER_API_RLIB) $(USER_RUNTIME_RLIB)
	mkdir -p $(dir $@)
	rustup run stable rustc --crate-name $* --edition 2021 --target $(TARGET) \
		--extern imagineos=$(USER_API_RLIB) --extern imagineos_rt=$(USER_RUNTIME_RLIB) \
		-L dependency=$(USER_API_DEPS) -L dependency=$(USER_RUNTIME_DEPS) \
		-C panic=abort -C relocation-model=static \
		-C link-arg=-Tuserland/linker.ld $< -o $@

.build/user/bin/%: userland/%.rs userland/linker.ld $(USER_API_RLIB) $(USER_RUNTIME_RLIB)
	mkdir -p $(dir $@)
	rustup run stable rustc --crate-name $* --edition 2021 --target $(TARGET) \
		--extern imagineos=$(USER_API_RLIB) --extern imagineos_rt=$(USER_RUNTIME_RLIB) \
		-L dependency=$(USER_API_DEPS) -L dependency=$(USER_RUNTIME_DEPS) \
		-C panic=abort -C relocation-model=static \
		-C link-arg=-Tuserland/linker.ld $< -o $@

.build/user/utilities/%: userland/utilities/%.rs userland/utilities/common.rs userland/linker.ld $(USER_API_RLIB) $(USER_RUNTIME_RLIB)
	mkdir -p $(dir $@)
	rustup run stable rustc --crate-name $* --edition 2021 --target $(TARGET) \
		--extern imagineos=$(USER_API_RLIB) --extern imagineos_rt=$(USER_RUNTIME_RLIB) \
		-L dependency=$(USER_API_DEPS) -L dependency=$(USER_RUNTIME_DEPS) \
		-C panic=abort -C relocation-model=static \
		-C link-arg=-Tuserland/linker.ld $< -o $@

$(USER_API_RLIB): $(USER_API_MANIFEST) userland/api/src/lib.rs \
	userland/api/src/args.rs userland/api/src/console.rs userland/api/src/fs.rs \
	userland/api/src/kernel_log.rs userland/api/src/legacy.rs \
	userland/api/src/process.rs userland/api/src/shutdown.rs userland/api/src/signals.rs \
	userland/api/src/syscall.rs userland/api/src/users.rs \
	shared/abi/Cargo.toml shared/abi/src/lib.rs
	rustup run stable cargo build --manifest-path $(USER_API_MANIFEST) \
		--target $(TARGET) --release --target-dir $(USER_API_TARGET_DIR)

$(USER_RUNTIME_RLIB): $(USER_RUNTIME_MANIFEST) userland/runtime/src/lib.rs $(USER_API_RLIB)
	rustup run stable cargo build --manifest-path $(USER_RUNTIME_MANIFEST) \
		--target $(TARGET) --release --target-dir $(USER_RUNTIME_TARGET_DIR)

iso: kernel $(RAMFS_IMAGE)
	command -v xorriso >/dev/null
	command -v mkfs.vfat >/dev/null
	command -v mcopy >/dev/null
	command -v mmd >/dev/null
	rm -rf $(ISO_DIR)
	mkdir -p $(ISO_DIR)/EFI/BOOT $(ISO_DIR)/boot distro
	cp $(BOOTSTRAP) $(ISO_DIR)/boot/bootstrap.elf
	cp $(DZ_IMAGE) $(ISO_DIR)/boot/dzImage
	cp $(RAMFS_IMAGE) $(ISO_DIR)/boot/ramfs.tar
	cp limine.conf $(ISO_DIR)/limine.conf
	cp toolchain/limine-binary/BOOTX64.EFI $(ISO_DIR)/EFI/BOOT/BOOTX64.EFI
	dd if=/dev/zero of=$(ISO_DIR)/efi.img bs=1M count=64
	mkfs.vfat $(ISO_DIR)/efi.img
	mmd -i $(ISO_DIR)/efi.img ::/EFI ::/EFI/BOOT ::/boot
	mcopy -i $(ISO_DIR)/efi.img $(ISO_DIR)/EFI/BOOT/BOOTX64.EFI ::/EFI/BOOT/
	mcopy -i $(ISO_DIR)/efi.img $(ISO_DIR)/limine.conf ::/limine.conf
	mcopy -i $(ISO_DIR)/efi.img $(ISO_DIR)/boot/bootstrap.elf ::/boot/bootstrap.elf
	mcopy -i $(ISO_DIR)/efi.img $(ISO_DIR)/boot/dzImage ::/boot/dzImage
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

disk-image: iso .build/dfs-image
	if [ -e "$(DISK_IMAGE)" ]; then \
		printf 'Using existing disk image: %s\n' "$(DISK_IMAGE)"; \
	else \
		sh tools/install-disk.sh "$(DISK_IMAGE)" \
			"$(ISO_DIR)/boot/bootstrap.elf" "$(ISO_DIR)/boot/dzImage" \
			"$(RAMFS_INSTALLED_IMAGE)" \
			"$(ISO_DIR)/EFI/BOOT/BOOTX64.EFI" "$(ISO_DIR)/limine.conf" \
			"tools/startup.nsh" ".build/dfs-image"; \
	fi

run-disk: disk-image
	test -f "$(OVMF_CODE)"
	$(QEMU) -machine pc -m 512M -serial stdio \
		-drive if=pflash,format=raw,unit=0,readonly=on,file="$(OVMF_CODE)" \
		-drive if=ide,index=0,format=raw,file="$(DISK_IMAGE)"

installer-disk:
	mkdir -p .build
	if [ ! -e "$(INSTALL_TARGET_IMAGE)" ]; then \
		qemu-img create -f raw "$(INSTALL_TARGET_IMAGE)" 1G; \
	fi

run-installer: iso installer-disk
	test -f "$(OVMF_CODE)"
	$(QEMU) -machine pc -m 512M -serial stdio -boot order=d \
		-drive if=pflash,format=raw,unit=0,readonly=on,file="$(OVMF_CODE)" \
		-drive if=ide,index=0,format=raw,file="$(INSTALL_TARGET_IMAGE)" \
		-cdrom $(ISO_IMAGE)

clean:
	rm -rf target .build
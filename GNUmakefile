SHELL := /bin/sh

TARGET := x86_64-unknown-none
KERNEL := target/$(TARGET)/release/dreamcore
ISO_DIR := .build/iso
INITRD := .build/initrd.tar
USER_PROGRAMS := .build/user/init.elf .build/user/getty.elf .build/user/shell.elf
ISO_IMAGE := distro/dreamcore-$(shell date +%Y-%m-%d-%H-%M)-astrid.iso
QEMU := qemu-system-x86_64
OVMF_CODE ?= /usr/share/OVMF/OVMF_CODE.fd

.PHONY: all kernel user-programs iso run clean

all: kernel

kernel: linker.ld
	rustup run stable cargo build --release --target $(TARGET)

$(INITRD): $(USER_PROGRAMS)
	mkdir -p .build/initrd
	cp .build/user/init.elf .build/initrd/init.elf
	cp .build/user/getty.elf .build/initrd/getty.elf
	cp .build/user/shell.elf .build/initrd/shell.elf
	tar --format=ustar -C .build/initrd -cf $@ init.elf getty.elf shell.elf

.PHONY: user-programs
user-programs: $(USER_PROGRAMS)

.build/user/%.elf: userspace/%.rs userspace/linker.ld
	mkdir -p .build/user
	rustup run stable rustc --crate-name $* --edition 2021 --target $(TARGET) \
		-C panic=abort -C relocation-model=static \
		-C link-arg=-Tuserspace/linker.ld $< -o $@

iso: kernel $(INITRD)
	command -v xorriso >/dev/null
	command -v mkfs.vfat >/dev/null
	command -v mcopy >/dev/null
	command -v mmd >/dev/null
	mkdir -p $(ISO_DIR)/EFI/BOOT $(ISO_DIR)/boot distro
	cp $(KERNEL) $(ISO_DIR)/boot/kernel.elf
	cp $(INITRD) $(ISO_DIR)/boot/initrd.tar
	cp limine.conf $(ISO_DIR)/limine.conf
	cp toolchain/limine-binary/BOOTX64.EFI $(ISO_DIR)/EFI/BOOT/BOOTX64.EFI
	dd if=/dev/zero of=$(ISO_DIR)/efi.img bs=1M count=16
	mkfs.vfat $(ISO_DIR)/efi.img
	mmd -i $(ISO_DIR)/efi.img ::/EFI ::/EFI/BOOT ::/boot
	mcopy -i $(ISO_DIR)/efi.img $(ISO_DIR)/EFI/BOOT/BOOTX64.EFI ::/EFI/BOOT/
	mcopy -i $(ISO_DIR)/efi.img $(ISO_DIR)/limine.conf ::/limine.conf
	mcopy -i $(ISO_DIR)/efi.img $(ISO_DIR)/boot/kernel.elf ::/boot/kernel.elf
	mcopy -i $(ISO_DIR)/efi.img $(ISO_DIR)/boot/initrd.tar ::/boot/initrd.tar
	xorriso -as mkisofs -R -r -J -V DREAMCORE \
		--efi-boot efi.img -efi-boot-part --efi-boot-image \
		--protective-msdos-label $(ISO_DIR) -o $(ISO_IMAGE)
	@printf 'ISO gerada: %s\n' '$(ISO_IMAGE)'

run: iso
	test -f "$(OVMF_CODE)"
	$(QEMU) -machine q35 -m 512M -serial stdio -bios "$(OVMF_CODE)" -cdrom $(ISO_IMAGE)

clean:
	rm -rf target .build
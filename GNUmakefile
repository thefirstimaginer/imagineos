SHELL := /bin/sh

KERNEL_NAME := dreamcore
CODENAME := astrid

TARGET := x86_64-unknown-none
KERNEL_FEATURES ?=
KERNEL := target/$(TARGET)/release/dreamcore
ISO_DIR := .build/iso
RAMFS_IMAGE := .build/ramfs.tar
USER_UTILITIES := cat grep ls mkdir rm touch
USER_C_APPS ?= vim
RAMFS_DIRS := bin home system/fonts tmp usr

USER_PROGRAMS := .build/user/init.elf .build/user/getty.elf .build/user/shell.elf .build/user/tcc.elf \
	$(addprefix .build/user/utilities/,$(addsuffix .elf,$(USER_UTILITIES))) \
	$(addprefix .build/user/c/,$(addsuffix .elf,$(USER_C_APPS)))

RAMFS_FILES := $(shell find ramfs -type f | sort)
ISO_IMAGE := distro/$(KERNEL_NAME)-$(shell date +%Y-%m-%d-%H-%M)-$(CODENAME).iso
QEMU := qemu-system-x86_64
OVMF_CODE ?= /usr/share/OVMF/OVMF_CODE.fd
TCC_SOURCE := third_party/tinycc
TCC_HOST := .build/toolchain/tcc
TCC_CFLAGS := -mno-sse -B$(TCC_SOURCE) -Iuserspace/libc/include
MUSL_SOURCE := third_party/musl-1.2.6

.PHONY: all kernel user-programs iso run clean musl-source

all: kernel

kernel: linker.ld
	rustup run stable cargo build --release --target $(TARGET) $(if $(KERNEL_FEATURES),--features $(KERNEL_FEATURES),)

$(RAMFS_IMAGE): $(USER_PROGRAMS) $(RAMFS_FILES)
	rm -rf .build/ramfs
	mkdir -p $(addprefix .build/ramfs/,$(RAMFS_DIRS))
	cp -a ramfs/. .build/ramfs/
	cp .build/user/init.elf .build/ramfs/bin/init
	cp .build/user/getty.elf .build/ramfs/bin/getty
	cp .build/user/shell.elf .build/ramfs/bin/shell
	cp .build/user/tcc.elf .build/ramfs/bin/tcc
	for utility in $(USER_UTILITIES); do cp .build/user/utilities/$$utility.elf .build/ramfs/bin/$$utility; done
	for app in $(USER_C_APPS); do cp .build/user/c/$$app.elf .build/ramfs/bin/$$app; done
	tar --format=ustar -C .build/ramfs -cf $@ bin home system tmp usr

.PHONY: user-programs
user-programs: $(USER_PROGRAMS)

APP ?= vim
.PHONY: c-app
c-app: .build/user/c/$(APP).elf

$(TCC_SOURCE)/Makefile:
	mkdir -p third_party
	git clone --depth 1 --branch mob https://repo.or.cz/tinycc.git $(TCC_SOURCE)

$(MUSL_SOURCE)/README: .build/musl-1.2.6.tar.gz
	mkdir -p third_party
	tar -xzf $< -C third_party

.build/musl-1.2.6.tar.gz:
	mkdir -p .build
	curl -fL https://musl.libc.org/releases/musl-1.2.6.tar.gz -o $@

musl-source: $(MUSL_SOURCE)/README

.build/user/%.elf: userspace/%.rs userspace/linker.ld
	mkdir -p .build/user
	rustup run stable rustc --crate-name $* --edition 2021 --target $(TARGET) \
		-C panic=abort -C relocation-model=static \
		-C link-arg=-Tuserspace/linker.ld $< -o $@

.build/user/tcc.elf: userspace/tcc.c userspace/libc/include/dreamcore.h userspace/libc/include/string.h userspace/libc/include/stdio.h userspace/libc/include/stdlib.h userspace/libc/include/unistd.h userspace/libc/include/fcntl.h .build/user/crt0.o .build/user/unistd.o .build/user/string.o .build/user/stdio.o .build/user/stdlib.o $(TCC_HOST) $(TCC_SOURCE)/libtcc1.a userspace/linker.ld
	mkdir -p .build/user
	$(TCC_HOST) $(TCC_CFLAGS) -c $< -o .build/user/tcc.o
	$(CC) -nostdlib -static -no-pie -Wl,-Tuserspace/linker.ld -Wl,--build-id=none .build/user/tcc.o .build/user/crt0.o .build/user/unistd.o .build/user/string.o .build/user/stdio.o .build/user/stdlib.o $(TCC_SOURCE)/libtcc1.a -o $@

.build/user/utilities/%.elf: userspace/utilities/%.rs userspace/utilities/common.rs userspace/linker.ld
	mkdir -p .build/user/utilities
	rustup run stable rustc --crate-name $* --edition 2021 --target $(TARGET) \
		-C panic=abort -C relocation-model=static \
		-C link-arg=-Tuserspace/linker.ld $< -o $@

.build/toolchain/tcc: $(TCC_SOURCE)/Makefile $(TCC_SOURCE)/tcc.c
	mkdir -p .build/toolchain
	cd $(TCC_SOURCE) && ./configure --prefix="$(abspath .build/toolchain/install)"
	$(MAKE) -C $(TCC_SOURCE)
	cp $(TCC_SOURCE)/tcc $@

$(TCC_SOURCE)/libtcc1.a: $(TCC_HOST)
	@test -f $@

.build/user/unistd.o: userspace/libc/unistd.c userspace/libc/include/unistd.h userspace/libc/include/fcntl.h userspace/libc/include/dreamcore.h
	mkdir -p .build/user
	$(TCC_HOST) $(TCC_CFLAGS) -c $< -o $@

.build/user/string.o: userspace/libc/string.c userspace/libc/include/string.h
	mkdir -p .build/user
	$(TCC_HOST) $(TCC_CFLAGS) -c $< -o $@

.build/user/stdio.o: userspace/libc/stdio.c userspace/libc/include/stdio.h userspace/libc/include/unistd.h
	mkdir -p .build/user
	$(TCC_HOST) $(TCC_CFLAGS) -c $< -o $@

.build/user/stdlib.o: userspace/libc/stdlib.c userspace/libc/include/stdlib.h userspace/libc/include/string.h userspace/libc/include/unistd.h
	mkdir -p .build/user
	$(TCC_HOST) $(TCC_CFLAGS) -c $< -o $@

.build/user/crt0.o: userspace/libc/crt0.c userspace/libc/include/dreamcore.h $(TCC_HOST)
	mkdir -p .build/user
	$(TCC_HOST) $(TCC_CFLAGS) -c $< -o $@

.build/user/c/%.elf: ramfs/home/%.c userspace/libc/include/dreamcore.h userspace/libc/include/string.h userspace/libc/include/stdio.h userspace/libc/include/stdlib.h userspace/libc/include/unistd.h userspace/libc/include/fcntl.h .build/user/crt0.o .build/user/unistd.o .build/user/string.o .build/user/stdio.o .build/user/stdlib.o $(TCC_HOST) $(TCC_SOURCE)/libtcc1.a userspace/linker.ld
	mkdir -p .build/user/c
	$(TCC_HOST) $(TCC_CFLAGS) -c $< -o .build/user/c/$*.o
	$(CC) -nostdlib -static -no-pie -Wl,-Tuserspace/linker.ld -Wl,--build-id=none .build/user/c/$*.o .build/user/crt0.o .build/user/unistd.o .build/user/string.o .build/user/stdio.o .build/user/stdlib.o $(TCC_SOURCE)/libtcc1.a -o $@

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
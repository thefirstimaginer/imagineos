SHELL := /bin/sh

# ==============================================================================
# CONFIGURAÇÕES E VARIÁVEIS GERAIS
# ==============================================================================

K_NAME         := dreamcore
K_VERSION      := 0.1.0
CODENAME       := astrid
DISTRIBUTION   := imagineos

TARGET         := x86_64-unknown-none
KERNEL_FEATURES ?=

# Ferramentas e Executáveis
QEMU           := qemu-system-x86_64
OVMF_CODE      := OVMF.fd
CARGO          := rustup run stable cargo
RUSTC          := rustup run stable rustc

# ==============================================================================
# DIRETÓRIOS E ARQUIVOS DE SAÍDA (.build / .build/distro)
# ==============================================================================

BUILD_DIR      := .build
DISTRO_DIR     := $(BUILD_DIR)/distro

KERNEL         := target/$(TARGET)/release/dreamcore
BOOTSTRAP      := $(BUILD_DIR)/bootstrap.elf
DZ_IMAGE       := $(BUILD_DIR)/dzImage
RAMFS_IMAGE    := $(BUILD_DIR)/ramfs.tar
RAMFS_INST_IMG := $(BUILD_DIR)/ramfs-installed.tar
ISO_DIR        := $(BUILD_DIR)/iso
ISO_IMAGE      := $(DISTRO_DIR)/$(K_NAME)-$(shell date +%Y-%m-%d-%H-%M)-$(CODENAME).iso
DISK_IMAGE     := $(BUILD_DIR)/imagineos-disk.img
INSTALL_TARGET := $(BUILD_DIR)/installer-target.img

# Ferramentas auxiliares compiladas no host
DZPACK_BIN     := $(BUILD_DIR)/dzpack
DFS_IMAGE_BIN  := $(BUILD_DIR)/dfs-image
TCC_HOST       := $(BUILD_DIR)/toolchain/tcc

# Fontes de Terceiros e Recursos Gerais
LIMINE_DIR     := third_party/limine
TCC_SOURCE     := third_party/tinycc
TCC_CONFIG_STAMP := $(BUILD_DIR)/toolchain/tinycc-config.stamp
TCC_PATCH_STAMP := $(BUILD_DIR)/toolchain/tinycc-imagineos-patched.stamp
TCC_SOURCE_REV := 43c7708b85681a2fd4451c8a541af4494a8919b2
FONT_ASSET     := tools/assets/zap-vga16.psf

# ==============================================================================
# CONFIGURAÇÃO DO RAMFS E PROGRAMAS DE USUÁRIO
# ==============================================================================

RAMFS_ROOT     := ramfs
RAMFS_DIRS     := bin dev sbin home system/fonts system/install tmp usr
USER_UTILITIES := cat dmesg distroinstall fdtest globalconf grep kill ls mkdir ps rm shutdown su sudo touch uname vi hello
USER_C_UTILITIES         := hello_c tcc

# Manifestos e Libs do Userland
USER_API_MANIFEST       := userland/api/Cargo.toml
USER_API_TARGET_DIR     := $(BUILD_DIR)/user/api-target
USER_API_RLIB           := $(USER_API_TARGET_DIR)/$(TARGET)/release/libimagineos.rlib
USER_API_DEPS           := $(USER_API_TARGET_DIR)/$(TARGET)/release/deps

USER_RUNTIME_MANIFEST   := userland/runtime/Cargo.toml
USER_RUNTIME_TARGET_DIR := $(BUILD_DIR)/user/runtime-target
USER_RUNTIME_RLIB       := $(USER_RUNTIME_TARGET_DIR)/$(TARGET)/release/libimagineos_rt.rlib
USER_RUNTIME_DEPS       := $(USER_RUNTIME_TARGET_DIR)/$(TARGET)/release/deps

USER_PROGRAMS           := $(BUILD_DIR)/user/sbin/init \
                           $(BUILD_DIR)/user/sbin/getty \
                           $(BUILD_DIR)/user/bin/shell \
                           $(addprefix $(BUILD_DIR)/user/utilities/,$(USER_UTILITIES) $(USER_C_UTILITIES))

USER_CFLAGS             := -ffreestanding -fno-stack-protector -fno-pic -fno-pie \
                           -fno-builtin -mno-red-zone -nostdinc -Iuserland/libc/include \
                           -isystem $(shell $(CC) -print-file-name=include)
USER_CRT_OBJECTS        := $(BUILD_DIR)/user/crt0.o $(BUILD_DIR)/user/runtime.o \
                           $(BUILD_DIR)/user/unistd.o \
                           $(BUILD_DIR)/user/path.o $(BUILD_DIR)/user/string.o $(BUILD_DIR)/user/stdio.o \
                           $(BUILD_DIR)/user/stdlib.o $(BUILD_DIR)/user/time.o \
                           $(BUILD_DIR)/user/math.o $(BUILD_DIR)/user/mman.o \
                           $(BUILD_DIR)/user/signals.o $(BUILD_DIR)/user/setjmp.o \
                           $(BUILD_DIR)/user/signal.o
USER_LIBC_OBJECTS       := $(filter-out $(BUILD_DIR)/user/crt0.o,$(USER_CRT_OBJECTS))
TCC_NATIVE_SOURCES      := tcc.c libtcc.c tccpp.c tccgen.c tccdbg.c \
                           tccelf.c tccasm.c tccrun.c x86_64-gen.c x86_64-link.c \
                           i386-asm.c
TCC_NATIVE_OBJECTS      := $(patsubst %.c,$(BUILD_DIR)/user/tcc/%.o,$(TCC_NATIVE_SOURCES))
TCC_NATIVE_CFLAGS       := $(USER_CFLAGS) -I$(TCC_SOURCE) -I$(TCC_SOURCE)/include \
                           -DTCC_TARGET_X86_64 -DCONFIG_TCC_STATIC -DCONFIG_TCC_SEMLOCK=0 \
                           -DONE_SOURCE=0 \
                           -DCONFIG_TCC_SYSINCLUDEPATHS=\"/usr/include\" \
                           -DCONFIG_TCC_LIBPATHS=\"/usr/lib\" \
                           -DCONFIG_TCC_CRTPREFIX=\"/usr/lib\"

# ==============================================================================
# REGRAS PHONY
# ==============================================================================

.PHONY: all kernel bootstrap dzimage ramfs user-programs iso run \
        disk-image run-disk installer-disk run-installer clean

all: kernel

# ==============================================================================
# COMPILAÇÃO DO KERNEL E BOOTSTRAP
# ==============================================================================

kernel: $(KERNEL)

$(KERNEL): kernel/linker.ld $(FONT_ASSET)
	$(CARGO) build --release --target $(TARGET) --bin dreamcore \
		--config 'target.x86_64-unknown-none.rustflags=["-C","link-arg=-Tkernel/linker.ld","-C","relocation-model=static"]' \
		$(if $(KERNEL_FEATURES),--features $(KERNEL_FEATURES),)

bootstrap: $(BOOTSTRAP)

$(BOOTSTRAP): bootstrap/bootstrap.rs bootstrap/boot_info.rs bootstrap/dzimage.rs \
              kernel/src/time.rs kernel/src/console/framebuffer.rs \
              $(FONT_ASSET) boot/bootstrap.ld
	$(CARGO) build --release --target $(TARGET) --bin bootstrap \
		--features bootstrap \
		--target-dir $(BUILD_DIR)/bootstrap-target \
		--config 'target.x86_64-unknown-none.rustflags=["-C","link-arg=-Tboot/bootstrap.ld","-C","relocation-model=static"]'
	@mkdir -p $(BUILD_DIR)
	cp $(BUILD_DIR)/bootstrap-target/$(TARGET)/release/bootstrap $@

# ==============================================================================
# FERRAMENTAS DO HOST (TOOLS)
# ==============================================================================

$(DZPACK_BIN): tools/dzpack.rs bootstrap/dzimage.rs
	@mkdir -p $(BUILD_DIR)
	$(RUSTC) --edition 2021 $< -o $@

$(DFS_IMAGE_BIN): tools/dfs-image.rs kernel/src/fs/dfs.rs kernel/src/fs/gpt.rs kernel/src/drivers/block.rs
	@mkdir -p $(BUILD_DIR)
	$(RUSTC) --edition 2021 $< -o $@

dzimage: $(DZ_IMAGE)

$(DZ_IMAGE): $(KERNEL) $(DZPACK_BIN)
	$(DZPACK_BIN) $(KERNEL) $@

# ==============================================================================
# GERAÇÃO DA ESTRUTURA E IMAGEM DO RAMFS
# ==============================================================================

# Cria a estrutura base do RAMFS na raiz do projeto
ramfs:
	@mkdir -p $(addprefix $(RAMFS_ROOT)/,$(RAMFS_DIRS))

# Prepara e empacota o tar do RAMFS no .build
$(RAMFS_IMAGE): ramfs kernel $(BOOTSTRAP) $(DZ_IMAGE) $(USER_PROGRAMS) \
                $(BUILD_DIR)/user/libdreamcore.a $(BUILD_DIR)/user/crt_empty.o \
                $(TCC_SOURCE)/libtcc1.a \
                $(LIMINE_DIR)/BOOTX64.EFI boot/limine.conf boot/startup.nsh
	rm -rf $(BUILD_DIR)/ramfs
	mkdir -p $(BUILD_DIR)/ramfs
	cp -a $(RAMFS_ROOT)/. $(BUILD_DIR)/ramfs/
	cp $(FONT_ASSET) $(BUILD_DIR)/ramfs/system/fonts/zap-vga16.psf
	mkdir -p $(BUILD_DIR)/ramfs/usr/include $(BUILD_DIR)/ramfs/usr/lib/tcc
	cp -a userland/libc/include/. $(BUILD_DIR)/ramfs/usr/include/
	cp $(BUILD_DIR)/user/libdreamcore.a $(BUILD_DIR)/ramfs/usr/lib/libdreamcore.a
	cp $(BUILD_DIR)/user/libdreamcore.a $(BUILD_DIR)/ramfs/usr/lib/libc.a
	$(AR) rcs $(BUILD_DIR)/user/libm.a $(BUILD_DIR)/user/math.o
	cp $(BUILD_DIR)/user/libm.a $(BUILD_DIR)/ramfs/usr/lib/libm.a
	cp $(BUILD_DIR)/user/crt0.o $(BUILD_DIR)/ramfs/usr/lib/crt0.o
	cp $(BUILD_DIR)/user/crt0.o $(BUILD_DIR)/ramfs/usr/lib/crt1.o
	cp $(BUILD_DIR)/user/crt_empty.o $(BUILD_DIR)/ramfs/usr/lib/crti.o
	cp $(BUILD_DIR)/user/crt_empty.o $(BUILD_DIR)/ramfs/usr/lib/crtn.o
	cp userland/linker.ld $(BUILD_DIR)/ramfs/usr/lib/imagineos.ld
	cp $(TCC_SOURCE)/libtcc1.a $(BUILD_DIR)/ramfs/usr/lib/tcc/libtcc1.a
	
	# Binários userland
	cp $(BUILD_DIR)/user/sbin/init $(BUILD_DIR)/ramfs/sbin/init
	cp $(BUILD_DIR)/user/sbin/getty $(BUILD_DIR)/ramfs/sbin/getty
	cp $(BUILD_DIR)/user/bin/shell $(BUILD_DIR)/ramfs/bin/shell
	for utility in $(USER_UTILITIES); do \
		cp $(BUILD_DIR)/user/utilities/$$utility $(BUILD_DIR)/ramfs/bin/$$utility; \
	done
	for utility in $(USER_C_UTILITIES); do \
		cp $(BUILD_DIR)/user/utilities/$$utility $(BUILD_DIR)/ramfs/bin/$$utility; \
	done

	# Tar para o instalador do SO
	tar --format=ustar --numeric-owner --owner=0 --group=0 \
		--exclude=bin/distroinstall --exclude='system/install/*' \
		-C $(BUILD_DIR)/ramfs -cf $(RAMFS_INST_IMG) bin dev sbin home system tmp usr

	# Adiciona os arquivos do instalador
	mkdir -p $(BUILD_DIR)/ramfs/system/install
	cp $(BOOTSTRAP) $(BUILD_DIR)/ramfs/system/install/bootstrap.elf
	cp $(DZ_IMAGE) $(BUILD_DIR)/ramfs/system/install/dzImage
	cp $(LIMINE_DIR)/BOOTX64.EFI $(BUILD_DIR)/ramfs/system/install/BOOTX64.EFI
	cp boot/limine.conf $(BUILD_DIR)/ramfs/system/install/limine.conf
	cp boot/startup.nsh $(BUILD_DIR)/ramfs/system/install/startup.nsh
	cp $(RAMFS_INST_IMG) $(BUILD_DIR)/ramfs/system/install/ramfs-installed.tar

	# Tar final do RAMFS
	tar --format=ustar --numeric-owner --owner=0 --group=0 \
		-C $(BUILD_DIR)/ramfs -cf $@ bin dev sbin home system tmp usr

# ==============================================================================
# USERLAND (APPS & UTILITIES EM RUST)
# ==============================================================================

user-programs: $(USER_PROGRAMS)

$(USER_API_RLIB): $(USER_API_MANIFEST) $(wildcard userland/api/src/*.rs) shared/abi/Cargo.toml shared/abi/src/lib.rs
	$(CARGO) build --manifest-path $(USER_API_MANIFEST) \
		--target $(TARGET) --release --target-dir $(USER_API_TARGET_DIR)

$(USER_RUNTIME_RLIB): $(USER_RUNTIME_MANIFEST) userland/runtime/src/lib.rs $(USER_API_RLIB)
	$(CARGO) build --manifest-path $(USER_RUNTIME_MANIFEST) \
		--target $(TARGET) --release --target-dir $(USER_RUNTIME_TARGET_DIR)

# Apps principais (init, getty, shell) em userland/apps/
$(BUILD_DIR)/user/sbin/%: userland/apps/%.rs userland/linker.ld $(USER_API_RLIB) $(USER_RUNTIME_RLIB)
	@mkdir -p $(dir $@)
	$(RUSTC) --crate-name $* --edition 2021 --target $(TARGET) \
		--extern imagineos=$(USER_API_RLIB) --extern imagineos_rt=$(USER_RUNTIME_RLIB) \
		-L dependency=$(USER_API_DEPS) -L dependency=$(USER_RUNTIME_DEPS) \
		-C panic=abort -C relocation-model=static \
		-C link-arg=-Tuserland/linker.ld $< -o $@

$(BUILD_DIR)/user/bin/%: userland/apps/%.rs userland/linker.ld $(USER_API_RLIB) $(USER_RUNTIME_RLIB)
	@mkdir -p $(dir $@)
	$(RUSTC) --crate-name $* --edition 2021 --target $(TARGET) \
		--extern imagineos=$(USER_API_RLIB) --extern imagineos_rt=$(USER_RUNTIME_RLIB) \
		-L dependency=$(USER_API_DEPS) -L dependency=$(USER_RUNTIME_DEPS) \
		-C panic=abort -C relocation-model=static \
		-C link-arg=-Tuserland/linker.ld $< -o $@

# Utilitários CLI em userland/utilities/
$(BUILD_DIR)/user/utilities/%: userland/utilities/%.rs userland/utilities/common.rs userland/linker.ld $(USER_API_RLIB) $(USER_RUNTIME_RLIB)
	@mkdir -p $(dir $@)
	$(RUSTC) --crate-name $* --edition 2021 --target $(TARGET) \
		--extern imagineos=$(USER_API_RLIB) --extern imagineos_rt=$(USER_RUNTIME_RLIB) \
		-L dependency=$(USER_API_DEPS) -L dependency=$(USER_RUNTIME_DEPS) \
		-C panic=abort -C relocation-model=static \
		-C link-arg=-Tuserland/linker.ld $< -o $@

# ==============================================================================
# USERLAND (SUITE C / TCC / LIBC)
# ==============================================================================

$(BUILD_DIR)/user/%.o: userland/libc/%.c $(wildcard userland/libc/include/*.h userland/libc/include/sys/*.h)
	@mkdir -p $(dir $@)
	$(CC) $(USER_CFLAGS) -c $< -o $@

$(BUILD_DIR)/user/crt0.o: userland/libc/crt0.S
	@mkdir -p $(dir $@)
	$(CC) $(USER_CFLAGS) -c $< -o $@

$(BUILD_DIR)/user/runtime.o: userland/libc/crt0.c
	@mkdir -p $(dir $@)
	$(CC) $(USER_CFLAGS) -c $< -o $@

$(BUILD_DIR)/user/crt_empty.o: userland/libc/crt_empty.S
	@mkdir -p $(dir $@)
	$(CC) $(USER_CFLAGS) -c $< -o $@

$(BUILD_DIR)/user/setjmp.o: userland/libc/setjmp.S
	@mkdir -p $(dir $@)
	$(CC) $(USER_CFLAGS) -c $< -o $@

$(BUILD_DIR)/user/signal.o: userland/libc/signal.S
	@mkdir -p $(dir $@)
	$(CC) $(USER_CFLAGS) -c $< -o $@

$(BUILD_DIR)/user/libdreamcore.a: $(USER_LIBC_OBJECTS)
	@mkdir -p $(dir $@)
	$(AR) rcs $@ $^

$(BUILD_DIR)/user/utilities/hello_c: userland/utilities/hello_c.c userland/linker.ld \
                                     $(USER_CRT_OBJECTS)
	@mkdir -p $(dir $@)
	$(CC) $(USER_CFLAGS) -nostdlib -static -no-pie \
		-Wl,-T,userland/linker.ld -Wl,--build-id=none \
		$< $(USER_CRT_OBJECTS) -o $@

$(TCC_SOURCE)/configure:
	@echo "TinyCC source is missing. Run 'make tcc-source' to fetch the pinned upstream revision."
	@false

.PHONY: tcc-source
tcc-source:
	@if [ ! -x "$(TCC_SOURCE)/configure" ]; then \
		git clone https://github.com/TinyCC/tinycc.git "$(TCC_SOURCE)" && \
		git -C "$(TCC_SOURCE)" checkout --detach "$(TCC_SOURCE_REV)"; \
	fi

$(TCC_CONFIG_STAMP): $(TCC_SOURCE)/configure Makefile
	@mkdir -p $(BUILD_DIR)/toolchain
	cd $(TCC_SOURCE) && ./configure --prefix=/usr --tccdir=/usr/lib/tcc \
		--sysincludepaths=/usr/include:/usr/include/x86_64-linux-gnu:$(shell $(CC) -print-file-name=include) \
		--libpaths=/usr/lib:/usr/lib/x86_64-linux-gnu \
		--crtprefix=/usr/lib/x86_64-linux-gnu:/usr/lib \
		--config-bcheck=no --config-backtrace=no
	touch $@

$(TCC_PATCH_STAMP): userland/patches/tinycc-imagineos.patch $(TCC_SOURCE)/tccrun.c $(TCC_CONFIG_STAMP)
	@if ! grep -Fq 'TCCSYM(gettimeofday)' $(TCC_SOURCE)/tccrun.c; then \
		patch --forward -p1 -d $(TCC_SOURCE) < $<; \
	fi
	touch $@

$(TCC_HOST): $(TCC_CONFIG_STAMP) $(TCC_PATCH_STAMP) $(TCC_SOURCE)/Makefile $(TCC_SOURCE)/tcc.c
	@mkdir -p $(BUILD_DIR)/toolchain
	$(MAKE) -C $(TCC_SOURCE)
	cp $(TCC_SOURCE)/tcc $@

$(TCC_SOURCE)/libtcc1.a: $(TCC_HOST)
	@test -f $@

$(BUILD_DIR)/user/tcc/%.o: $(TCC_SOURCE)/%.c $(TCC_HOST) $(TCC_PATCH_STAMP) \
                            $(wildcard userland/libc/include/*.h userland/libc/include/sys/*.h)
	@mkdir -p $(dir $@)
	$(CC) $(TCC_NATIVE_CFLAGS) -c $< -o $@

$(BUILD_DIR)/user/utilities/tcc: $(TCC_NATIVE_OBJECTS) $(USER_CRT_OBJECTS) userland/linker.ld
	@mkdir -p $(dir $@)
	$(CC) $(USER_CFLAGS) -nostdlib -static -no-pie \
		-Wl,-T,userland/linker.ld -Wl,--build-id=none \
		$(TCC_NATIVE_OBJECTS) $(USER_CRT_OBJECTS) -o $@

$(BUILD_DIR)/user/c/%.elf: ramfs/home/%.c userland/libc/include/dreamcore.h \
                            $(USER_CRT_OBJECTS) \
                            userland/linker.ld
	@mkdir -p $(BUILD_DIR)/user/c
	$(CC) $(USER_CFLAGS) -c $< -o $(BUILD_DIR)/user/c/$*.o
	$(CC) $(USER_CFLAGS) -nostdlib -static -no-pie -Wl,-Tuserland/linker.ld \
		-Wl,--build-id=none $(BUILD_DIR)/user/c/$*.o $(USER_CRT_OBJECTS) -o $@

# ==============================================================================
# GERAÇÃO DA ISO E QEMU
# ==============================================================================

iso: kernel $(RAMFS_IMAGE)
	@command -v xorriso >/dev/null || (echo "xorriso não encontrado"; exit 1)
	@command -v mkfs.vfat >/dev/null || (echo "mkfs.vfat não encontrado"; exit 1)
	@command -v mcopy >/dev/null || (echo "mcopy não encontrado"; exit 1)
	@command -v mmd >/dev/null || (echo "mmd não encontrado"; exit 1)
	rm -rf $(ISO_DIR)
	mkdir -p $(ISO_DIR)/EFI/BOOT $(ISO_DIR)/boot $(DISTRO_DIR)
	cp $(BOOTSTRAP) $(ISO_DIR)/boot/bootstrap.elf
	cp $(DZ_IMAGE) $(ISO_DIR)/boot/dzImage
	cp $(RAMFS_IMAGE) $(ISO_DIR)/boot/ramfs.tar
	cp boot/limine.conf $(ISO_DIR)/limine.conf
	cp $(LIMINE_DIR)/BOOTX64.EFI $(ISO_DIR)/EFI/BOOT/BOOTX64.EFI
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
	@printf 'ISO gerada com sucesso: %s\n' '$(ISO_IMAGE)'

run: iso
	@test -f "$(OVMF_CODE)" || (echo "OVMF firmware não encontrado em $(OVMF_CODE)"; exit 1)
	$(QEMU) -machine q35 -m 512M -serial stdio \
		-drive if=pflash,format=raw,unit=0,readonly=on,file="$(OVMF_CODE)" \
		-cdrom $(ISO_IMAGE)

disk-image: iso $(DFS_IMAGE_BIN)
	@if [ -e "$(DISK_IMAGE)" ]; then \
		printf 'Usando imagem de disco existente: %s\n' "$(DISK_IMAGE)"; \
	else \
		sh tools/install-disk.sh "$(DISK_IMAGE)" \
			"$(ISO_DIR)/boot/bootstrap.elf" "$(ISO_DIR)/boot/dzImage" \
			"$(RAMFS_INST_IMG)" \
			"$(ISO_DIR)/EFI/BOOT/BOOTX64.EFI" "$(ISO_DIR)/limine.conf" \
			"boot/startup.nsh" "$(DFS_IMAGE_BIN)"; \
	fi

run-disk: disk-image
	@test -f "$(OVMF_CODE)" || (echo "OVMF firmware não encontrado em $(OVMF_CODE)"; exit 1)
	$(QEMU) -machine pc -m 512M -serial stdio \
		-drive if=pflash,format=raw,unit=0,readonly=on,file="$(OVMF_CODE)" \
		-drive if=ide,index=0,format=raw,file="$(DISK_IMAGE)"

installer-disk:
	@mkdir -p $(BUILD_DIR)
	@if [ ! -e "$(INSTALL_TARGET)" ]; then \
		qemu-img create -f raw "$(INSTALL_TARGET)" 1G; \
	fi

run-installer: iso installer-disk
	@test -f "$(OVMF_CODE)" || (echo "OVMF firmware não encontrado em $(OVMF_CODE)"; exit 1)
	$(QEMU) -machine pc -m 512M -serial stdio -boot order=d \
		-drive if=pflash,format=raw,unit=0,readonly=on,file="$(OVMF_CODE)" \
		-drive if=ide,index=0,format=raw,file="$(INSTALL_TARGET)" \
		-cdrom $(ISO_IMAGE)

# ==============================================================================
# LIMPEZA
# ==============================================================================

clean:
	rm -rf target $(BUILD_DIR)
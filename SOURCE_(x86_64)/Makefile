CC := x86_64-elf-gcc
LD := x86_64-elf-ld

kernel_source_files := $(shell find source/implementation/kernel -name *.c)
kernel_object_files := $(patsubst source/implementation/kernel/%.c, build/kernel/%.o, $(kernel_source_files))

x86_64_c_source_files := $(shell find source/implementation/x86_64 -name *.c)
x86_64_c_object_files := $(patsubst source/implementation/x86_64/%.c, build/x86_64/%.o, $(x86_64_c_source_files))

x86_64_asm_source_files := $(shell find source/implementation/x86_64 -name *.asm)
x86_64_asm_object_files := $(patsubst source/implementation/x86_64/%.asm, build/x86_64/%.o, $(x86_64_asm_source_files))

x86_64_object_files := $(x86_64_c_object_files) $(x86_64_asm_object_files)

build/kernel/%.o: source/implementation/kernel/%.c
	mkdir -p $(dir $@)
	$(CC) -c -I source/include -ffreestanding $(patsubst build/kernel/%.o, source/implementation/kernel/%.c, $@) -o $@

build/x86_64/%.o: source/implementation/x86_64/%.c
	mkdir -p $(dir $@)
	$(CC) -c -I source/include -ffreestanding $(patsubst build/x86_64/%.o, source/implementation/x86_64/%.c, $@) -o $@

build/x86_64/%.o: source/implementation/x86_64/%.asm
	mkdir -p $(dir $@)
	nasm -f elf64 $(patsubst build/x86_64/%.o, source/implementation/x86_64/%.asm, $@) -o $@

.PHONY: build-x86_64
build-x86_64: $(kernel_object_files) $(x86_64_object_files)
	mkdir -p distro/x86_64
	$(LD) -n -o distro/x86_64/kernel.bin -T targets/x86_64/linker.ld $(kernel_object_files) $(x86_64_object_files)
	cp distro/x86_64/kernel.bin targets/x86_64/iso/boot/kernel.bin
	grub-mkrescue /usr/lib/grub/i386-pc -o distro/x86_64/ImagineOS-READY.iso targets/x86_64/iso

.PHONY: clean
clean:
	rm -rf build distro

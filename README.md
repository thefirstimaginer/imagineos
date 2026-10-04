# Imagine Operating System

ImagineOS is an experimental educational x86_64 operating system, codenamed **Astrid**. The kernel is freestanding Rust and uses the Limine boot protocol. This is an early bring-up, not yet a general-purpose OS.

## Build

Requirements: Rust stable with the `x86_64-unknown-none` target, GNU Make, Limine's x86_64 UEFI executable at `toolchain/limine-binary/BOOTX64.EFI`, `xorriso`, `dosfstools`, `mtools`, and QEMU with OVMF for boot testing.

```sh
rustup target add x86_64-unknown-none
make kernel
make iso
```

The UEFI-only image is written to `distro/dreamcore-YYYY-MM-DD-HH-MM-astrid.iso`.

## Boot State

The kernel consumes Limine's HHDM, memory map, framebuffer, and RAMFS module. It installs a GDT/TSS and fatal exception IDT, initializes a 4 KiB frame allocator and a 1 MiB bump heap, mounts the USTAR archive, and loads `/bin/init`. Init starts `/bin/getty`, which starts `/bin/shell`; the shell resolves external commands under `/bin`.

The shell provides `cd`, `pwd`, `echo`, `export`, `unset`, `set`, `read`, `clear`, `pid`, `type`, and `exit`. External commands are searched through `PATH` and launched from `/bin`; initial read-only utilities include `ls`, `cat`, and fixed-string `grep`. `argv` and exported environment entries are passed to child ELFs. PSF/PSF2 fonts are searched in `ramfs/system/fonts`; a built-in framebuffer font is used when none can be loaded. The prompt cursor blinks while input is polled.

This is a small shell, not a full POSIX language implementation: pipelines, redirection, aliases, functions, and control-flow syntax are not supported. The current USTAR filesystem is read-only, so `cp`, `mv`, and `rm` are not provided. Scheduling is cooperative round-robin; timer preemption, heap reclamation, and full W^X permissions are also pending.

More details: [documentation/README.md](documentation/README.md).

## Copyright

    Copyright (C) 2024-2026 The Imagine Project & Adryan Alcantara
    
    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.
    
    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.
    
    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.

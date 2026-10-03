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

The kernel consumes Limine's HHDM, memory map, framebuffer, and initrd responses. It installs a GDT/TSS and fatal exception IDT, initializes a 4 KiB frame allocator and a 1 MiB bump heap, then loads three ELF64 programs from a USTAR RAMFS. `init`, `getty`, and `shell` run in ring 3 with separate page-table roots.

The kernel provides `int 0x80` syscalls for console I/O, yield, exit, PID, and clear. Scheduling is cooperative round-robin; timer preemption, heap reclamation, full W^X permissions, VFS, and persistent storage are not implemented yet.

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

# Imagine Operating System

ImagineOS is an experimental educational x86_64 operating system, codenamed **Astrid**. The kernel is freestanding Rust and uses the Limine boot protocol. This is an early bring-up, not yet a general-purpose OS.

## Build

Requirements: Rust stable with the `x86_64-unknown-none` target, GNU Make, Limine's x86_64 UEFI executable at `toolchain/limine-binary/BOOTX64.EFI`, `xorriso`, `dosfstools`, `mtools`, and QEMU with OVMF for boot testing.

```sh
rustup target add x86_64-unknown-none
mkdir -p ramfs
make kernel
make iso
```

Create the `ramfs/` directory at the repository root before building the image;
it is the source tree used to assemble the filesystem archive. The UEFI-only
image is written to `distro/dreamcore-YYYY-MM-DD-HH-MM-astrid.iso`.

## Boot State

The kernel consumes Limine's HHDM, memory map, framebuffer, and RAMFS module. It installs a GDT/TSS and fatal exception IDT, initializes a 4 KiB frame allocator and a 1 MiB bump heap, mounts the USTAR archive, and loads `/sbin/init` as PID 1. Init starts `/sbin/getty`, which starts `/bin/shell`; the shell resolves external commands under `/bin`.

Kernel sources live in `dnu/`; userspace programs (including init and getty)
live in `userland/`. The USTAR module serves as the initial RAM filesystem;
there is no disk-backed root filesystem or `switch_root` implementation yet.

The shell provides `cd`, `pwd`, `echo`, `export`, `unset`, `set`, `read`, `clear`, `pid`, `type`, and `exit`. External commands are searched through `PATH` and launched from `/bin`; utilities include `ls`, `cat`, fixed-string `grep`, `mkdir`, `touch`, `rm`, `vi`, and `globalconf`. `argv` and exported environment entries are passed to child ELFs. PSF/PSF2 fonts are searched in `ramfs/system/fonts`; built-in ASCII/Portuguese glyphs are used when a loaded font lacks a character. The prompt cursor blinks while input is polled.

This is a small shell, not a full POSIX language implementation: pipelines, redirection, aliases, functions, and control-flow syntax are not supported. Rust `std` is unnecessary: the kernel remains `no_std` and exposes OS operations through its own syscalls. The USTAR base stays immutable; `mkdir`, `touch`, and `rm` update a bounded in-memory overlay and changes disappear at reboot. Scheduling is cooperative round-robin; timer preemption, heap reclamation, and full W^X permissions are also pending.

Rust user programs can use the reusable `no_std` crate in `userland/api/`.
Syscall numbers and shared data structures live in `shared/abi/`; ABI v1 and
its error and argument conventions are documented in
[documentation/syscall-abi.md](documentation/syscall-abi.md). This userspace
API is ImagineOS-specific and does not provide Rust `std` or POSIX compatibility.

More details: [documentation/README.md](documentation/README.md), including the
[boot flow](documentation/boot-flow.md) and
[userspace program development guide](documentation/userspace.md).

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

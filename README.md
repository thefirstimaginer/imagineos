# Imagine Operating System <img width="200" height="auto" align="right" alt="imagine3w" src="https://github.com/user-attachments/assets/e406f860-0038-4076-8014-8fe5f7ff955e" />

ImagineOS is an experimental and educational x86_64 operating system. The current
release is vR1, codenamed **Jessica**.

The project is under construction and unstable. It currently boots through GRUB
and Multiboot2, starts a freestanding 64-bit kernel, and provides a VGA text-mode
shell for testing kernel and userspace components.

## Current status

- GRUB/Multiboot2 boot sequence
- x86_64 kernel written in C and Assembly
- IDT, PIC, timer, RTC, PS/2 keyboard, and VGA text output
- Basic process management and round-robin scheduler
- QBshell with command history
- No filesystem or persistent storage
- No UEFI boot support
- No active graphics/framebuffer mode

## Requirements

On a Debian or Ubuntu-based system, install the toolchain and emulator first:

```sh
sudo apt install build-essential nasm binutils grub-pc-bin grub-common xorriso qemu-system-x86_64
```

The build uses `gcc`, `ld`, `nasm`, and `grub-mkrescue`. QEMU is only required
to run the resulting image.

## Build and run

Build the kernel ELF:

```sh
make
```

Create a bootable ISO at `distro/imos.iso`:

```sh
make iso
```

Build the ISO and start it in QEMU with serial output connected to the terminal:

```sh
make run
```

`make qemu` is an alias for `make run`. To remove generated files:

```sh
make clean
```

The generated kernel ELF is `.build/imos.elf`. The ISO is created with GRUB and
boots in legacy BIOS mode; UEFI is not supported yet.

## Shell commands

The current shell registers these commands:

`calc`, `clear`, `halt`, `help`, `history`, `li`, `liteinterp`, `ps`, `ver`, and
`video`.

Command arguments and some command implementations are still limited. Use
`history` to navigate previously entered commands.

## Project layout

- `arch/x86_64/boot/`: Multiboot header, boot assembly, and linker script
- `arch/x86_64/src/`: interrupt, PIC, and port implementations
- `kernel/`: kernel entry point, modules, processes, and scheduler
- `devices/drivers/`: keyboard, PS/2, RTC, video, and text output drivers
- `init/`: system initialization and initial processes
- `userspace/`: shell and user-facing modules
- `include/` and `lib/`: headers and freestanding library code
- `documentation/`: design notes and development roadmaps

For planned work and known limitations, see [TODO.md](TODO.md) and the
[development roadmap](documentation/roadmap.md).

## Copyright

    Copyright (C) 2024-2026 TeamImagine
    
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

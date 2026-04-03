# Imagine Operating System

## Prerequisites

 - Visual Studio or any text editor
 - NASM - Netwide ASeMbler
 - GRUB or GRUB2
 - Xorriso

## Build
Build for x86 (other architectures may come in the future):
 - `make build-x86_64`
 - If you are using Qemu, please close it before running this command to prevent errors.

## Emulate
You can emulate your operating system using [Qemu](https://www.qemu.org/).

 - `qemu-system-x86_64 -cdrom dist/x86_64/kernel.iso`
 - Note: Close the emulator when finished, so as to not block writing to `kernel.iso` for future builds.

Alternatively, you should be able to load the operating system on a USB drive and boot into it when you turn on your computer. (I tested it, you need to boot in Legacy Mode, if you're in a UEFI system.)

## Copyright

    Copyright (C) 2024-2026  The Imagine System Project
    
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

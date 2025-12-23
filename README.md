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

Alternatively, you should be able to load the operating system on a USB drive and boot into it when you turn on your computer. (I haven't actually tested this yet.)

Copyright © 2024-2025 Imagine Technologies, All Rights Reserved.

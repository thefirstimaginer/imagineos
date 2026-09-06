# Imagine Operating System <img width="200" height="auto" align="right" alt="imagine3w" src="https://github.com/user-attachments/assets/e406f860-0038-4076-8014-8fe5f7ff955e" />



## Version Release 1

ImagineOS is an experimental operating system and is still unstable. The
current image is generated as `distro/imos.iso`.

## Build

Requirements: GCC, NASM, GNU ld, `grub-mkrescue`, and QEMU.

```sh
make clean
make iso
```

The generated ISO is:

```text
distro/imos.iso
```

## Run

Run the ISO with the Makefile target:

```sh
make run
```

For a headless run:

```sh
timeout 8s qemu-system-x86_64 \
    -no-reboot -display none -serial stdio \
    -cdrom distro/imos.iso
```

The current system can boot the kernel, load userspace ELF modules, and run a
text shell in successful executions. Userspace process lifecycle and paging
remain under development; `init.elf` and `proc-test` can still cause faults or
restart QEMU.

More detailed documentation is available in [documentation/README.md](documentation/README.md).

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

# Testes

## Compilacao e parser USTAR

```sh
rustup run stable cargo check --target x86_64-unknown-none
rustup run stable cargo build --release --target x86_64-unknown-none
rustup run stable rustc --edition 2021 --test src/ramfs.rs -o /tmp/dreamcore-ramfs-tests
/tmp/dreamcore-ramfs-tests
rustup run stable rustc --edition 2021 --test src/elf.rs -o /tmp/dreamcore-elf-tests
/tmp/dreamcore-elf-tests
make user-programs
make .build/initrd.tar
tar -tf .build/initrd.tar
```

Para criar a ISO, instale `xorriso`, `dosfstools` e `mtools`, depois rode `make iso`.

## Boot manual

Use QEMU com firmware OVMF (UEFI), conecte COM1 ao terminal e inicialize a ISO. O prompt oferece `help`, `clear`, `pid`, `echo` e `exit`.

Sem QEMU/OVMF ou sem as ferramentas de ISO, compilacao e testes locais nao comprovam um boot real.

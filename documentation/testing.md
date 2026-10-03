# Testes

## Compilacao e parser USTAR

```sh
rustup run stable cargo check --target x86_64-unknown-none
rustup run stable cargo build --release --target x86_64-unknown-none
rustup run stable rustc --test src/ramfs.rs -o /tmp/dreamcore-ramfs-tests
/tmp/dreamcore-ramfs-tests
make .build/initrd.tar
tar -tf .build/initrd.tar
```

Para criar a ISO, instale `xorriso`, `dosfstools` e `mtools`, depois rode `make iso`.

## Boot manual

Use QEMU com firmware OVMF (UEFI), conecte COM1 ao terminal e inicialize a ISO. O prompt oferece `help`, `clear`, `ls`, `cat /init`, `mem`, `ps` e `echo`.

Sem QEMU/OVMF ou sem as ferramentas de ISO, compilacao e testes locais nao comprovam um boot real.

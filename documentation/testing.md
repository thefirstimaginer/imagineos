# Testes

## Compilacao e parser USTAR

```sh
rustup run stable cargo check --target x86_64-unknown-none
rustup run stable cargo build --release --target x86_64-unknown-none
rustup run stable rustc --edition 2021 --test src/fs/ramfs.rs -o /tmp/dreamcore-ramfs-tests
/tmp/dreamcore-ramfs-tests
rustup run stable rustc --edition 2021 --test src/exec/elf.rs -o /tmp/dreamcore-elf-tests
/tmp/dreamcore-elf-tests
make user-programs
make .build/ramfs.tar
tar -tf .build/ramfs.tar
```

O tar deve conter `bin/init`, `bin/getty`, `bin/shell`, `bin/ls`, `bin/cat`, `bin/grep` e `system/fonts/default8x9.psf`.

Para criar a ISO, instale `xorriso`, `dosfstools` e `mtools`, depois rode `make iso`.
Para reativar logs de cada etapa de carregamento: `make KERNEL_FEATURES=kernel-debug iso`.

## Boot manual

Use QEMU com firmware OVMF (UEFI), conecte COM1 ao terminal e inicialize a ISO. Teste `pwd`, `cd /bin`, `ls` (deve listar o `/bin` atual), `ls .`, `cd /home` (vazio no RAMFS inicial), `export X=astrid`, `echo "$X"`, `type ls`, `cat /bin/init`, `grep Astrid /bin/init` e `read NAME`. No prompt, backspace deve remover o glifo inteiro, incluindo caracteres UTF-8.

Sem QEMU/OVMF ou sem as ferramentas de ISO, compilacao e testes locais nao comprovam um boot real.

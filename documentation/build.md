# Build e execucao

## Dependencias

- Rust stable e `rustup target add x86_64-unknown-none`;
- GNU Make e `rustup` no `PATH`;
- `xorriso`, `dosfstools` (`mkfs.vfat`) e `mtools` (`mmd`, `mcopy`) para gerar ISO;
- `toolchain/limine-binary/BOOTX64.EFI`;
- QEMU x86_64 e OVMF para testar o boot UEFI.

## Compilar a ISO

Na raiz do repositorio:

```sh
make kernel
make iso
```

A ISO UEFI sera gerada em `distro/dreamcore-AAAA-MM-DD-HH-MM-astrid.iso`.
O arquivo USTAR `ramfs.tar` e produzido da arvore `ramfs/`; os ELFs compilados
de `userspace/*.rs` sao colocados em `ramfs/bin/` no staging antes do tar.

Apps C em `ramfs/home/<nome>.c` implementam `main`; `make c-app APP=<nome>` os
compila pelo TCC upstream no host e linka como ELF estatico para o Dreamcore.
`USER_C_APPS="vim outro" make .build/ramfs.tar` empacota os ELFs em `/bin`; o
padrao inclui `vim`, cujo fonte `ramfs/home/vim.c` tambem fica no tar. A
libc fornece o entrypoint `_start` por `crt0.c`. A interface inicial em
`userspace/libc/` inclui `read`, `write`, `_exit`, `printf`, `puts`, entrada e
saida de caracteres, operacoes basicas de `<string.h>` e `<stdlib.h>`, e as
extensoes Dreamcore para ler/gravar arquivos inteiros. O formatador cobre
`%s`, `%c`, inteiros decimais/hexadecimais e `%%`; o allocator e um bump arena
de 64 KiB por processo e `free` ainda nao recupera memoria. I/O de arquivo esta
limitado a 4096 bytes por chamada; a area mutavel total da ramfs e 1 MiB.

Este ainda nao e um port de runtime do TCC nem da musl. O TCC roda no host para
produzir o ELF, e `third_party/musl-1.2.6` e apenas fonte de referencia: o build
nativo dela usa syscalls Linux e nao e linkado aos apps Dreamcore. TCC e musl sao
compativeis entre si em Linux, mas o ImagineOS precisa de mais syscalls POSIX,
descritores de arquivo, memoria dinamica e startup antes de executar esses
projetos upstream dentro da ramfs. As fontes ignoradas podem ser recuperadas
com `make musl-source`; o build do app baixa o TCC para `third_party/` quando
essa arvore ainda nao existe.

## Executar com QEMU

```sh
make run
```

Configure `OVMF_CODE` se o firmware nao estiver no caminho padrao. A imagem
atual nao inclui boot BIOS. O ESP contem o kernel e `ramfs.tar`; os programas
userspace existem somente dentro do RAMFS.

# Build e execução

## Dependências

- Rust stable e `rustup target add x86_64-unknown-none`;
- GNU Make e `rustup` no `PATH`;
- `xorriso`, `dosfstools` (`mkfs.vfat`) e `mtools` (`mmd`, `mcopy`) para gerar ISO;
- `third_party/limine/BOOTX64.EFI`;
- QEMU x86_64 e OVMF para testar o boot UEFI.
- `qemu-img`, `sgdisk`, `mkfs.vfat` e `mtools` para criar a imagem de disco GPT.

## Compilar a ISO

Na raiz do repositório, crie `ramfs/` caso ainda não exista. Essa pasta é a
árvore de origem do filesystem incluído na imagem; a etapa de empacotamento
falha se ela estiver ausente.

```sh
mkdir -p ramfs
make kernel
make bootstrap dzimage
make iso
```

A ISO UEFI será gerada em `.build/distro/dreamcore-AAAA-MM-DD-HH-MM-astrid.iso`.
`make bootstrap` cria o ELF inicial do Limine e `make dzimage` comprime o ELF
do kernel em `.build/dzImage` usando o empacotador LZ4 próprio. O módulo traz
magic, tamanho descomprimido e CRC32. Durante o boot, o bootstrap atualiza a
porcentagem de descompressão na mesma linha, carrega os segmentos e então
transfere as respostas Limine ao kernel. O USTAR `ramfs.tar` é produzido a
partir de `ramfs/`; os programas Rust de `userland/apps/` e
`userland/utilities/` são incluídos sem
extensão: `/sbin/init`, `/sbin/getty` e comandos em `/bin`. A fonte PSF
8x16 `tools/assets/zap-vga16.psf` é incorporada ao kernel e ao bootstrap como
fallback para uma fonte inválida ou ausente no RAMFS. Ela também é copiada
para `system/fonts/zap-vga16.psf` dentro do USTAR, de modo que o instalador
inclui a fonte na raiz DFS persistente.

Apps C em `ramfs/home/<nome>.c` implementam `main`; `make c-app APP=<nome>` os
compila pelo TCC upstream no host e linka como ELF estatico para o Dreamcore.
`USER_C_APPS="vim outro" make .build/ramfs.tar` empacota os ELFs em `/bin`; o
padrao inclui `vim`, cujo fonte `ramfs/home/vim.c` tambem fica no tar. A
libc fornece o entrypoint `_start` por `crt0.c`. A interface inicial em
`userland/libc/` inclui `read`, `write`, `_exit`, `printf`, `puts`, entrada e
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

Configure `OVMF_CODE` se o firmware não estiver no caminho padrão, por exemplo:
`make run OVMF_CODE=/usr/share/OVMF/OVMF_CODE_4M.fd`. A imagem atual não inclui
boot BIOS. O ESP contém `bootstrap.elf`, `dzImage` e `ramfs.tar`; os programas
de userspace existem somente dentro do RAMFS.

## Criar e iniciar uma imagem de disco GPT

`make disk-image` cria `.build/imagineos-disk.img` sem sobrescrever uma imagem
existente. A ferramenta `tools/install-disk.sh` grava uma tabela GPT, uma ESP
FAT16 de 128 MiB com Limine, bootstrap, `dzImage` e `ramfs.tar`, além de uma
partição DFS no espaço restante. O formatador host `.build/dfs-image` importa
na partição DFS o USTAR do sistema instalado para que ela possa ser usada como
raiz persistente. O alvo não altera discos físicos.

```sh
make disk-image
make run-disk
```

`make run-disk` inicia a imagem em QEMU/OVMF com chipset PC/IDE, sem anexar a
ISO. O kernel deve detectar o disco ATA primary-master, localizar e montar a
partição DFS pela GPT, e carregar `/sbin/init` dela. O USTAR continua montado
como initramfs e fallback se não houver DFS montável. Para recriar a imagem,
remova somente `.build/imagineos-disk.img` e rode `make disk-image` novamente.

Para testar o instalador que roda no userspace, use `make run-installer`. Ele
inicia a ISO e anexa `.build/installer-target.img`, uma imagem de 1 GiB criada
apenas se ainda não existir. Execute `distroinstall` no shell, informe
opcionalmente conta/senha/hostname e confirme o alvo com as duas frases
solicitadas; todos os dados da imagem alvo serão
apagados. Depois, `make run-disk DISK_IMAGE=.build/installer-target.img` testa
o boot pela instalação e a raiz DFS sem a ISO. Não use essa operação em um
disco físico com dados a preservar.

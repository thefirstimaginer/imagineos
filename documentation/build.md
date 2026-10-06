# Build e execução

## Dependências

- Rust stable e `rustup target add x86_64-unknown-none`;
- GNU Make e `rustup` no `PATH`;
- `xorriso`, `dosfstools` (`mkfs.vfat`) e `mtools` (`mmd`, `mcopy`) para gerar ISO;
- `toolchain/limine-binary/BOOTX64.EFI`;
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

A ISO UEFI será gerada em `distro/dreamcore-AAAA-MM-DD-HH-MM-astrid.iso`.
`make bootstrap` cria o ELF inicial do Limine e `make dzimage` comprime o ELF
do kernel em `.build/dzImage` usando o empacotador LZ4 próprio. O módulo traz
magic, tamanho descomprimido e CRC32. Durante o boot, o bootstrap atualiza a
porcentagem de descompressão na mesma linha, carrega os segmentos e então
transfere as respostas Limine ao kernel. O USTAR `ramfs.tar` é produzido a
partir de `ramfs/`; os programas Rust de `userland/` são incluídos sem
extensão: `/sbin/init`, `/sbin/getty` e comandos em `/bin`. A fonte PSF
8x16 `tools/fonts/zap-vga16.psf` é incorporada ao kernel e ao bootstrap como
fallback para uma fonte inválida ou ausente no RAMFS.

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
FAT16 de 128 MiB com Limine, bootstrap, `dzImage` e `ramfs.tar`, além de uma partição GPT
reservada ao DFS no espaço restante. O alvo não altera discos físicos.

```sh
make disk-image
make run-disk
```

`make run-disk` inicia a imagem em QEMU/OVMF com chipset PC/IDE, sem anexar a
ISO. O kernel deve detectar o disco ATA primary-master e localizar a partição DFS pela GPT.
Atualmente essa partição é apenas reservada: o sistema ainda inicia sua raiz
do `ramfs.tar` e não grava arquivos persistentes no DFS. Para recriar a imagem,
remova somente `.build/imagineos-disk.img` e rode `make disk-image` novamente.

Para testar o instalador que roda no userspace, use `make run-installer`. Ele
inicia a ISO e anexa `.build/installer-target.img`, uma imagem de 1 GiB criada
apenas se ainda não existir. Execute `distroinstall` no shell e confirme o
alvo com as duas frases solicitadas; todos os dados da imagem alvo serão
apagados. Depois, `make run-disk DISK_IMAGE=.build/installer-target.img` testa
o boot pela instalação. Não use essa operação em um disco físico com dados a
preservar.

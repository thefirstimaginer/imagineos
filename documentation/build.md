# Build e execução

## Dependências

- Rust stable e `rustup target add x86_64-unknown-none`;
- GNU Make e `rustup` no `PATH`;
- `xorriso`, `dosfstools` (`mkfs.vfat`) e `mtools` (`mmd`, `mcopy`) para gerar ISO;
- `toolchain/limine-binary/BOOTX64.EFI`;
- QEMU x86_64 e OVMF para testar o boot UEFI.

## Compilar a ISO

Na raiz do repositório, crie `ramfs/` caso ainda não exista. Essa pasta é a
árvore de origem do filesystem incluído na imagem; a etapa de empacotamento
falha se ela estiver ausente.

```sh
mkdir -p ramfs
make kernel
make iso
```

A ISO UEFI será gerada em `distro/dreamcore-AAAA-MM-DD-HH-MM-astrid.iso`.
O arquivo USTAR `ramfs.tar` é produzido a partir da árvore `ramfs/`. As fontes
do kernel ficam em `dnu/`; os programas Rust de `userland/` são compilados no
staging e incluídos sem extensão: `/sbin/init`, `/sbin/getty` e os comandos em
`/bin/`.

## Executar com QEMU

```sh
make run
```

Configure `OVMF_CODE` se o firmware não estiver no caminho padrão, por exemplo:
`make run OVMF_CODE=/usr/share/OVMF/OVMF_CODE_4M.fd`. A imagem atual não inclui
boot BIOS. O ESP contém o kernel e `ramfs.tar`; os programas de userspace
existem somente dentro do RAMFS.

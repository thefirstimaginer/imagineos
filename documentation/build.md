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

## Executar com QEMU

```sh
make run
```

Configure `OVMF_CODE` se o firmware nao estiver no caminho padrao. A imagem
atual nao inclui boot BIOS. O ESP contem o kernel e `ramfs.tar`; os programas
userspace existem somente dentro do RAMFS.

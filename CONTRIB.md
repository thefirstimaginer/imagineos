# Contribuindo

- Kernel: Rust stable, `no_std`, target `x86_64-unknown-none`.
- Valide com `rustup run stable cargo check --target x86_64-unknown-none` e `rustup run stable cargo build --release --target x86_64-unknown-none`.
- Para testar o parser RAMFS: `rustup run stable rustc --edition 2021 --test src/fs/ramfs.rs -o /tmp/dreamcore-ramfs-tests && /tmp/dreamcore-ramfs-tests`.
- O filesystem root da imagem vem de `ramfs/`; binarios Rust de usuario sao empacotados em `/bin` pelo alvo `make iso`.
- Utilitarios externos ficam em `userspace/utilities/` e sao copiados pelo build para `/bin`. Preserve built-ins e lookup PATH no shell; nao adicione comandos de escrita enquanto o RAMFS for somente leitura.
- Mudancas em boot, memoria ou interrupcoes tambem precisam de teste da ISO em QEMU/OVMF antes de serem consideradas validadas.
- Nao copie executaveis de usuario como arquivos independentes no ESP; o kernel os carrega do arquivo `ramfs.tar`.

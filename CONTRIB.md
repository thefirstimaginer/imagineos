# Contribuindo

- Kernel: Rust stable, `no_std`, target `x86_64-unknown-none`.
- Valide com `rustup run stable cargo check --target x86_64-unknown-none` e `rustup run stable cargo build --release --target x86_64-unknown-none`.
- Para testar o parser RAMFS: `rustup run stable rustc --test src/ramfs.rs -o /tmp/dreamcore-ramfs-tests && /tmp/dreamcore-ramfs-tests`.
- Mudancas em boot, memoria ou interrupcoes tambem precisam de teste da ISO em QEMU/OVMF antes de serem consideradas validadas.
- Nao descreva scripts de init ou comandos integrados como processos de usuario: ring 3 e loader ELF ainda nao existem.

# Contribuindo

- Kernel: Rust stable, `no_std`, target `x86_64-unknown-none`.
- Valide com `rustup run stable cargo check --target x86_64-unknown-none` e `rustup run stable cargo build --release --target x86_64-unknown-none`.
- Para testar o parser RAMFS: `rustup run stable rustc --edition 2021 --test dnu/fs/ramfs.rs -o /tmp/dreamcore-ramfs-tests && /tmp/dreamcore-ramfs-tests`.
- Antes de gerar a imagem, garanta que exista `ramfs/` na raiz do projeto. Se a pasta não existir, crie-a com `mkdir -p ramfs`; ela é a árvore de origem do sistema de arquivos da imagem.
- O sistema de arquivos raiz da imagem vem de `ramfs/`; binários Rust de usuário são empacotados em `/bin` pelo alvo `make iso`.
- O kernel fica em `dnu/`; programas de userspace, incluindo `init` e `getty`, ficam em `userland/`. Utilitários externos em `userland/utilities/` são copiados pelo build para `/bin`; `init` e `getty` são instalados em `/sbin` sem sufixo no nome.
- Preserve os comandos internos (built-ins) e a busca por `PATH` no shell; não adicione comandos de escrita enquanto o RAMFS for somente leitura.
- Mudanças em boot, memória ou interrupções também precisam de teste da ISO em QEMU/OVMF antes de serem consideradas validadas.
- Não copie executáveis de usuário como arquivos independentes no ESP; o kernel os carrega do arquivo `ramfs.tar`.

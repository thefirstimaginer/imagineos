# Testes

## Compilação e análise do parser USTAR

Para criar um utilitário de userspace, siga o
[guia de desenvolvimento de programas](userspace.md).

Antes de gerar o arquivo RAMFS, confirme que `ramfs/` existe na raiz do projeto.
Se necessário, crie a pasta com `mkdir -p ramfs`.

```sh
rustup run stable cargo check --target x86_64-unknown-none
rustup run stable cargo build --release --target x86_64-unknown-none
rustup run stable cargo test --manifest-path userland/api/Cargo.toml --target x86_64-unknown-linux-gnu
make user-programs
rustup run stable rustc --edition 2021 --test dnu/fs/ramfs.rs -o /tmp/dreamcore-ramfs-tests
/tmp/dreamcore-ramfs-tests
rustup run stable rustc --edition 2021 --test dnu/exec/elf.rs -o /tmp/dreamcore-elf-tests
/tmp/dreamcore-elf-tests
rustup run stable rustc --edition 2021 --test dnu/config.rs -o /tmp/dreamcore-config-tests
/tmp/dreamcore-config-tests
rustup run stable rustc --edition 2021 --test dnu/console/utf8.rs -o /tmp/dreamcore-utf8-tests
/tmp/dreamcore-utf8-tests
rustup run stable rustc --edition 2021 --test dnu/tests/keyboard.rs -o /tmp/dreamcore-keyboard-tests
/tmp/dreamcore-keyboard-tests
make .build/ramfs.tar
tar -tf .build/ramfs.tar
```

O tar deve conter `sbin/init`, `sbin/getty`, `bin/shell`, `bin/ls`, `bin/cat`,
`bin/grep`, `bin/globalconf`, `bin/vi`, `bin/mkdir`, `bin/rm`, `bin/touch`,
`bin/hello`, `home/.global/global.conf` e `system/fonts/zap-light16.psf`.

Para criar a ISO, instale `xorriso`, `dosfstools` e `mtools`, depois rode `make iso`.
Para reativar os logs de cada etapa de carregamento: `make KERNEL_FEATURES=kernel-debug iso`.

## Boot manual

Use QEMU com firmware OVMF (UEFI), conecte COM1 ao terminal e inicialize a ISO. Confirme que o kernel carrega `/sbin/init` como PID 1 e que init inicia getty/shell. Teste `globalconf show`, `globalconf get keyboard`, `globalconf set keyboard us`, `globalconf set keyboard abnt2`, `globalconf get charset`, `vi /home/.global/global.conf` (sair com `:q!`), `pwd`, `cd /bin`, `ls`, `ls /sbin`, `cd /home`, `export X=astrid`, `echo "$X"`, `type ls`, `hello`, `cat /home/readme.txt`, `grep ImagineOS /home/readme.txt` e `read NAME`. Com o layout ABNT2, teste `ç`, `á`, `ã`, `ê` e as setas no editor; no prompt, Backspace deve remover o glifo inteiro.

Sem QEMU/OVMF ou sem as ferramentas para gerar a ISO, a compilação e os testes locais não comprovam que o sistema inicializa corretamente.

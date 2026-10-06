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
rustup run stable rustc --edition 2021 --test dnu/tests/storage.rs -o /tmp/imagineos-storage-tests
/tmp/imagineos-storage-tests
rustup run stable rustc --edition 2021 --test dnu/tests/installer.rs -o /tmp/imagineos-installer-tests
/tmp/imagineos-installer-tests
rustup run stable rustc --edition 2021 --test dnu/dzimage.rs -o /tmp/imagineos-dzimage-tests
/tmp/imagineos-dzimage-tests
rustup run stable rustc --edition 2021 --test tools/dzpack.rs -o /tmp/imagineos-dzpack-tests
/tmp/imagineos-dzpack-tests
make .build/ramfs.tar
tar -tf .build/ramfs.tar
```

O tar deve conter `sbin/init`, `sbin/getty`, `bin/shell`, `bin/ls`, `bin/cat`,
`bin/grep`, `bin/globalconf`, `bin/distroinstall`, `bin/vi`, `bin/mkdir`, `bin/rm`, `bin/touch`,
`bin/hello`, `bin/fdtest`, `home/.global/global.conf` e
`system/fonts/zap-light16.psf`, além dos payloads em `system/install/`.
O RAMFS live fornece `system/install/bootstrap.elf` e `system/install/dzImage`;
o arquivo instalado não contém os payloads do instalador.

Para criar a ISO, instale `xorriso`, `dosfstools` e `mtools`, depois rode `make iso`.
Para reativar os logs de cada etapa de carregamento: `make KERNEL_FEATURES=kernel-debug iso`.

## Boot manual

Use QEMU com firmware OVMF (UEFI), conecte COM1 ao terminal e inicialize a ISO. Confirme que o kernel carrega `/sbin/init` como PID 1 e que init inicia getty/shell. Teste `globalconf show`, `globalconf get keyboard`, `globalconf set keyboard us`, `globalconf set keyboard abnt2`, `globalconf get charset`, `vi /home/.global/global.conf` (sair com `:q!`), `pwd`, `cd /bin`, `ls`, `ls /sbin`, `cd /home`, `export X=astrid`, `echo "$X"`, `type ls`, `hello`, `fdtest`, `cat /home/readme.txt`,
`grep ImagineOS /home/readme.txt` e `read NAME`. `fdtest` verifica stdio,
abertura, leitura, escrita, append, fechamento e erro de descritor fechado.
Com o layout ABNT2, teste `ç`, `á`, `ã`, `ê` e as setas no editor; no prompt,
Backspace deve remover o glifo inteiro.

`ls /dev` deve listar `/dev/hda` quando o disco IDE primary-master estiver
presente. Para validar a instalação, inicialize com `make run-installer`,
execute `distroinstall`, confirme digitando exatamente `APAGAR /dev/hda` e
`INSTALAR`, e reinicie com
`make run-disk DISK_IMAGE=.build/installer-target.img`. O utilitário apaga o
disco alvo; use apenas a imagem virtual de teste. A instalação deve chegar ao
shell sem a ISO e o RAMFS instalado não deve conter `/bin/distroinstall`.

Sem QEMU/OVMF ou sem as ferramentas para gerar a ISO, a compilação e os testes locais não comprovam que o sistema inicializa corretamente.

Para validar o boot por uma imagem de disco GPT em vez da ISO, use
`make disk-image` e `make run-disk`. Isso requer `qemu-img`, `sgdisk`,
`mkfs.vfat`, `mtools`, QEMU e OVMF. O teste cobre descoberta do disco ATA e da
partição GPT; não valida montagem nem persistência DFS, que ainda não foram
implementadas.

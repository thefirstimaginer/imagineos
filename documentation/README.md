# ImagineOS

ImagineOS e um sistema operacional experimental x86_64 com kernel freestanding em Rust, inicializado pelo protocolo Limine. O userspace inicial executa ELFs em ring 3 com scheduler cooperativo.

## Conteudo

- [Arquitetura](architecture.md)
- [Fluxo de boot e processo init](boot-flow.md)
- [Build e execucao](build.md)
- [Criar programas para o userspace](userspace.md)
- [Comandos do shell](commands.md)
- [Testes](testing.md)
- [Armazenamento, GPT e DFS](storage.md)
- [Estado atual e limitacoes](status.md)
- [Roadmap](roadmap.md)

## Estado resumido

O sistema ja possui:

- boot x86_64 UEFI pelo Limine;
- requisicoes Limine para HHDM, mapa de memoria, framebuffer e modulo RAMFS;
- GDT/TSS, IDT fatal para excecoes de CPU e allocator de frames;
- RAMFS USTAR, loader ELF64 e `/init -> /getty -> /shell` em ring 3;
- descoberta inicial do disco ATA em `/dev/hda` e utilitário `/bin/distroinstall`;
- page tables de usuario, heap bump de kernel, syscalls `int 0x80` e scheduler cooperativo;
- console serial, framebuffer, fonte embutida e carregador PSF1/PSF2;
- teclado PS/2 por polling com conversao UTF-8 para algumas teclas AltGr;
- shell de usuario com `help`, `clear`, `pid`, `echo` e `exit`.
- built-ins de sessao e utilitarios externos `ls`, `cat` e `grep` em `/bin`.

O acesso inicial a setores ATA e a leitura GPT já existem; timer/APIC,
preempção, reclaim de frames, W^X, VFS e armazenamento persistente ainda não.
O fluxo de boot e as limitações estão descritos em
[Fluxo de boot e processo init](boot-flow.md).

## Objetivo

A meta atual e validar e amadurecer o runtime inicial em QEMU: processos, paging, interrupcoes e ciclo de vida. Um display server ou driver grafico em userspace somente deve ser iniciado depois dessa base passar por testes repetidos sem reinicios.

# ImagineOS

ImagineOS e um sistema operacional experimental x86_64 com kernel freestanding em Rust, inicializado pelo protocolo Limine. A documentacao descreve o estado do bring-up Rust em outubro de 2026; o kernel ainda nao executa programas em ring 3.

## Conteudo

- [Arquitetura](architecture.md)
- [Build e execucao](build.md)
- [Comandos do shell](commands.md)
- [Testes](testing.md)
- [Estado atual e limitacoes](status.md)
- [Roadmap](roadmap.md)

## Estado resumido

O sistema ja possui:

- boot x86_64 UEFI pelo Limine;
- requisicoes Limine para HHDM, mapa de memoria, framebuffer e initrd;
- GDT/TSS, IDT fatal para excecoes de CPU e allocator de frames;
- RAMFS USTAR e cadeia `/init -> /getty -> /shell`;
- console serial, framebuffer, fonte embutida e carregador PSF1/PSF2;
- teclado PS/2 por polling com conversao UTF-8 para algumas teclas AltGr;
- shell de kernel com `help`, `clear`, `ls`, `cat /init`, `mem`, `ps` e `echo`.

Paging virtual, heap, carregamento ELF, ring 3, syscalls de usuario, scheduler e drivers de timer/APIC ainda nao existem.

## Objetivo

A meta atual e amadurecer o runtime de userspace: processos, paging, interrupcoes, syscalls e ciclo de vida. Um display server ou driver grafico em userspace somente deve ser iniciado depois dessa base passar por testes repetidos sem reinicios.

# ImagineOS

ImagineOS e um sistema operacional experimental x86_64, inicializado por Multiboot 2 e executado atualmente em modo BIOS/legacy com GRUB e QEMU.

O projeto esta em desenvolvimento ativo. A documentacao descreve o estado real do codigo em setembro de 2026, incluindo as partes funcionais e as falhas conhecidas.

## Conteudo

- [Arquitetura](architecture.md)
- [Build e execucao](build.md)
- [Estado atual e limitacoes](status.md)
- [Roadmap](roadmap.md)

## Estado resumido

O sistema ja possui:

- boot x86_64 em long mode;
- kernel freestanding em C e Assembly;
- GDT, TSS, IDT, PIC e timer PIT;
- paging inicial e espacos de endereco de userspace;
- carregamento de modulos ELF via Multiboot 2;
- entrada em ring 3;
- syscall ABI baseada em `syscall`/`sysret` manual com entrada em Assembly;
- libc minima para programas de usuario;
- `init`, shell e utilities como modulos separados;
- entrada PS/2 e terminal textual VGA;
- comandos `help`, `test`, `echo`, `clear`, `exit` e `proc-test` no shell.

O sistema ainda nao e estavel. O carregamento do `init.elf` pode falhar ou reiniciar a VM de forma intermitente, e `proc-test` pode causar triple fault/reinicio durante fork, troca de address space ou retorno do processo filho.

## Objetivo

A meta atual e amadurecer o runtime de userspace: processos, paging, interrupcoes, syscalls e ciclo de vida. Um display server ou driver grafico em userspace somente deve ser iniciado depois dessa base passar por testes repetidos sem reinicios.

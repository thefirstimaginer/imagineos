# Estado e limitacoes

## Implementado

- Entry point `no_std` x86_64 e requisicoes da crate `limine` 0.5.0.
- GDT de kernel, TSS com stack propria e IDT fatal para excecoes de CPU.
- Frame allocator monotonicamente crescente sobre regioes `USABLE` via HHDM.
- Console COM1, framebuffer RGB32, fonte 5x7 embutida e carregador PSF1/PSF2.
- Cursor do prompt piscante durante polling de entrada; fontes pesquisadas em `ramfs/system/fonts`.
- Polling de teclado PS/2 set-1, incluindo algumas teclas AltGr convertidas em Unicode.
- Heap global bump de 1 MiB e page tables user derivadas das mappings Limine.
- Parser ELF64 x86_64, loader de `PT_LOAD`, zero de BSS e stacks user.
- GDT ring 3, gate `int 0x80`, syscalls de I/O, yield, exit, PID e clear.
- Scheduler cooperativo e ELFs separados para init/getty/shell.
- RAMFS USTAR montado por path; init/getty/shell carregados de `/bin`, comandos externos resolvidos em `/bin`.
- Built-ins `cd`, `pwd`, `echo`, `export`, `unset`, `set`, `read`, `clear`, `pid`, `type` e `exit`; parser com aspas, escapes e expansão simples de variáveis.
- ELFs externos `/bin/ls`, `/bin/cat` e `/bin/grep`, com argv/envp, busca PATH e syscalls read-only do RAMFS.

## Ainda ausente ou nao validado

- ISO atual: `distro/dreamcore-2026-10-04-19-49-astrid.iso` (UEFI). O ESP contem apenas kernel e `ramfs.tar` em `/boot`, alem dos arquivos obrigatorios de boot Limine. O boot interativo desta versao ainda precisa ser validado em QEMU.
- O `#GP` observado no vetor 13 ocorria no `iretq`: `RAX` continha o ponteiro do TrapFrame, mas era sobrescrito com `0x33` antes de carregar `RSP`. A ordem foi corrigida e verificada no disassembly.
- Os escritores COM1 agora convertem LF isolado em CRLF, mantendo mensagens uma por linha.
- O PIC legado continua mascarado e IF desabilitado em ring 3 ate existir timer/APIC.
- Checkpoints `process:`/`elf:` ficam desabilitados no build normal; `make KERNEL_FEATURES=kernel-debug iso` os reativa para diagnostico.
- O scheduler nao e preemptivo; sem reclaim de frames, W^X, heap user, drivers de disco/rede, filesystem persistente ou layout de teclado completo.
- Shell ainda nao tem pipelines, redirecionamento, aliases, funcoes ou `if/for/while`. Sem filesystem gravavel, `cp`, `mv` e `rm` nao estao disponiveis; utilitarios de leitura tem limite de 4 KiB.

O check Rust e os testes locais ELF/USTAR passam, mas isso nao substitui o teste de boot real.

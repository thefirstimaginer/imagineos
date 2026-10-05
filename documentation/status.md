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
- Scheduler cooperativo e programas separados para init/getty/shell.
- RAMFS USTAR montado por path; `/sbin/init` inicia como PID 1, `/sbin/getty` inicia `/bin/shell`, e comandos externos são resolvidos em `/bin`.
- Built-ins `cd`, `pwd`, `echo`, `export`, `unset`, `set`, `read`, `clear`, `pid`, `type` e `exit`; parser com aspas, escapes e expansão simples de variáveis.
- Programas ELF externos em `/bin`, com `argv`/`envp`, busca por `PATH` e syscalls do RAMFS; init/getty e utilitários são fontes de userspace em `userland/`.

## Ainda ausente ou nao validado

- Build da ISO UEFI e boot interativo validados em QEMU/OVMF após a reorganização; o teste confirmou o início do kernel, a montagem do RAMFS, `/sbin/init` como PID 1 e o prompt do shell. O ESP contém kernel e `ramfs.tar` em `/boot`, além dos arquivos obrigatórios do Limine.
- O `#GP` observado no vetor 13 ocorria no `iretq`: `RAX` continha o ponteiro do TrapFrame, mas era sobrescrito com `0x33` antes de carregar `RSP`. A ordem foi corrigida e verificada no disassembly.
- Os escritores COM1 agora convertem LF isolado em CRLF, mantendo mensagens uma por linha.
- O PIC legado continua mascarado e IF desabilitado em ring 3 ate existir timer/APIC.
- Checkpoints `process:`/`elf:` ficam desabilitados no build normal; `make KERNEL_FEATURES=kernel-debug iso` os reativa para diagnostico.
- O scheduler nao e preemptivo; sem reclaim de frames, W^X, heap user, drivers de disco/rede, filesystem persistente ou layout de teclado completo.
- Shell ainda não tem pipelines, redirecionamento, aliases, funções ou `if/for/while`. Não há escrita persistente; `mkdir`, `touch` e `rm` alteram um overlay em RAM e as alterações se perdem no reboot. Utilitários de leitura têm limite de 4 KiB.

O check Rust e os testes locais ELF/USTAR passam, mas isso nao substitui o teste de boot real.

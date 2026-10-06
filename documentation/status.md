# Estado e limitacoes

## Implementado

- Entry point `no_std` x86_64 e requisicoes da crate `limine` 0.5.0.
- GDT de kernel, TSS com stack propria e IDT fatal para excecoes de CPU.
- Frame allocator monotonicamente crescente sobre regioes `USABLE` via HHDM.
- Logs do kernel com tempo decorrido em colchetes; console COM1 e framebuffer RGB32.
- Fontes PSF1/PSF2 pesquisadas em `ramfs/system/fonts`, com PSF2 embutida
  `tools/fonts/zap-vga32.psf` como fallback; caracteres ausentes usam `?`.
- Kernel panic explícita se `/sbin/init` não existir no RAMFS montado.
- Prefixos de tempo, fonte PSF2 de fallback e panic por `init` ausente
  verificados em boot QEMU/OVMF.
- Polling de teclado PS/2 set-1, layouts US/ABNT2, acentos compostos e eventos de setas.
- Heap global bump de 1 MiB e page tables user derivadas das mappings Limine.
- Parser ELF64 x86_64, loader de `PT_LOAD`, zero de BSS e stacks user.
- GDT ring 3, gate `int 0x80`, syscalls de I/O, yield, exit, PID e clear.
- ABI de syscalls v1 documentada e compartilhada entre kernel e crate `imagineos`; syscall 16 consulta a versão.
- Wrappers Rust `no_std` para console, RAMFS, processos e argumentos; init, getty, shell e utilitários usam a crate em vez de assembly duplicado.
- Tabelas limitadas por processo com stdio (FD 0-2), open/read/write/close para RAMFS e herança por cópia ao iniciar ELFs.
- `fdtest` validado no QEMU: stdio, open/read/write/append/close funcionam com as tabelas de descritores fora da estrutura de processo.
- Interface síncrona de setores de 512 bytes, driver ATA PIO do primary-master (LBA28) e parser da GPT primária com validação CRC32.
- Imagem GPT inicializável testada em QEMU/OVMF: o boot pelo disco confirmou ATA primary-master, 2.097.152 setores, localização da partição DFS via GPT e início do shell.
- Dispositivo ATA anunciado como `/dev/hda`, syscall de enumeração e `/bin/distroinstall` na mídia live; duas confirmações antes de criar GPT/ESP FAT16 e reservar o DFS.
- Instalação de ponta a ponta validada no QEMU: o utilitário gravou o disco virtual, a GPT passou `sgdisk -v`, o RAMFS instalado correspondeu ao payload live e o disco iniciou até o shell sem a ISO.
- Scheduler cooperativo e programas separados para init/getty/shell.
- RAMFS USTAR montado por path; `/sbin/init` inicia como PID 1, `/sbin/getty` inicia `/bin/shell`, e comandos externos são resolvidos em `/bin`.
- Built-ins `cd`, `pwd`, `echo`, `export`, `unset`, `set`, `read`, `clear`, `pid`, `type` e `exit`; parser com aspas, escapes e expansão simples de variáveis.
- Programas ELF externos em `/bin`, com `argv`/`envp`, busca por `PATH` e syscalls do RAMFS; init/getty e utilitários são fontes de userspace em `userland/`.
- `globalconf` lê/atualiza `/home/.global/global.conf`; o kernel aplica layout US/ABNT2 durante a sessão e UTF-8 é o charset aceito.
- `vi` modal com navegação, edição UTF-8 e gravação de arquivos de até 4 KiB no overlay RAMFS.

## Ainda ausente ou nao validado

- Build da ISO UEFI e boot interativo validados em QEMU/OVMF após a reorganização; o teste confirmou o início do kernel, a montagem do RAMFS, `/sbin/init` como PID 1, o prompt do shell e a execução bem-sucedida de `fdtest`. O ESP contém kernel e `ramfs.tar` em `/boot`, além dos arquivos obrigatórios do Limine.
- O `#GP` observado no vetor 13 ocorria no `iretq`: `RAX` continha o ponteiro do TrapFrame, mas era sobrescrito com `0x33` antes de carregar `RSP`. A ordem foi corrigida e verificada no disassembly.
- Os escritores COM1 agora convertem LF isolado em CRLF, mantendo mensagens uma por linha.
- O PIC legado continua mascarado e IF desabilitado em ring 3 ate existir timer/APIC.
- Checkpoints `process:`/`elf:` ficam desabilitados no build normal; `make KERNEL_FEATURES=kernel-debug iso` os reativa para diagnostico.
- O scheduler nao e preemptivo; sem reclaim de frames, W^X, heap user, drivers de rede ou suporte completo a layouts de teclado. O driver de disco cobre apenas IDE primário PIO/LBA28; a escrita foi validada pelo caminho do instalador, mas não há testes de energia/interrupção ou recuperação de falhas.
- Não há implementação do DFS/VFS nem persistência: a partição DFS criada pelo instalador permanece vazia, e o sistema continua montando o RAMFS inicial como raiz; portanto ainda não é possível carregar fontes nem a raiz persistente do disco.
- Shell ainda não tem pipelines, redirecionamento, aliases, funções ou `if/for/while`. Não há escrita persistente; `mkdir`, `touch`, `rm`, `vi` e `globalconf` alteram um overlay em RAM e as alterações se perdem no reboot. O único charset suportado é UTF-8 e o mapa ABNT2 cobre apenas as teclas implementadas pelo driver.

O check Rust e os testes locais ELF/USTAR passam, mas isso nao substitui o teste de boot real.

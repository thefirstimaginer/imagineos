# Estado e limitacoes

## Implementado

- Entry point `no_std` x86_64 e requisicoes da crate `limine` 0.5.0.
- GDT de kernel, TSS com stack propria e IDT que encerra isoladamente o processo
  ring 3 que causa uma excecao; excecoes no kernel continuam fatais.
- Frame allocator monotonicamente crescente sobre regioes `USABLE` via HHDM.
- Logs do kernel com tempo decorrido em colchetes; console COM1 e framebuffer RGB32.
- Buffer circular de 4 KiB para mensagens do kernel, syscall `dmesg` e utilitário
  `dmesg -n LINES`; logs de UEFI, Limine e bootstrap ainda ficam fora dele.
- Fonte padrão PSF1 8x16 `tools/fonts/zap-vga16.psf`; fontes PSF1/PSF2 do
  `ramfs/system/fonts` substituem o fallback; caracteres ausentes usam `?`.
- Kernel panic explícita se `/sbin/init` não existir no RAMFS montado.
- Prefixos de tempo, fonte PSF de fallback e panic por `init` ausente
  verificados em boot QEMU/OVMF.
- dzImage próprio com payload LZ4, tamanho/CRC32, bootstrap ELF pelo Limine,
  alocação e mapeamento dos segmentos do kernel, indicador de progresso na
  mesma linha e preservação das respostas Limine/reservas para o frame allocator,
  incluindo as page tables Limine compartilhadas pelos processos.
- Boot da ISO UEFI validado em QEMU/OVMF: o teste confirmou descompressão,
  init PID 1, getty e prompt interativo do shell; os comandos `pid` e `hello`
  foram executados após a correção das reservas das page tables.
- Polling de teclado PS/2 set-1, layouts US/ABNT2, acentos compostos e eventos de setas.
- Heap global bump de 1 MiB e page tables user derivadas das mappings Limine.
- Parser ELF64 x86_64, loader de `PT_LOAD`, zero de BSS e stacks user.
- GDT ring 3, gate `int 0x80`, syscalls de I/O, yield, exit, PID e clear.
- ABI de syscalls v2 documentada e compartilhada entre kernel e crate `imagineos`; syscall 16 consulta a versão.
- Wrappers Rust `no_std` para console, RAMFS, processos e argumentos; init, getty, shell e utilitários usam a crate em vez de assembly duplicado.
- Tabelas limitadas por processo com stdio (FD 0-2), open/read/write/close para RAMFS e herança por cópia ao iniciar ELFs.
- `fdtest` validado no QEMU: stdio, open/read/write/append/close funcionam com as tabelas de descritores fora da estrutura de processo.
- Interface síncrona de setores de 512 bytes, driver ATA PIO do primary-master (LBA28) e parser da GPT primária com validação CRC32.
- DFS persistente com superbloco/inodes/bitmap protegidos por checksum, extents, journal redo de metadados, replay no mount, diretórios, arquivos e metadados UID/GID/modo.
- Login/getty com UID/GID e hostname, conta adicional opcional com senha derivada por PBKDF2-HMAC-SHA-256, verificação de permissões owner/group/other e os utilitários `su`/`sudo`.
- Instalador e ferramenta host formatam a partição DFS e semeiam a árvore USTAR instalada; o boot tenta montar o DFS após o initramfs RAMFS e usa-o para carregar `/sbin/init`, mantendo RAMFS como fallback.
- A fachada RAMFS roteia operações para DFS quando montado; `ls` suporta metadados e oculta nomes iniciados por ponto por padrão (`-a` para mostrar).
- Imagem GPT inicializável testada em QEMU/OVMF: o boot pelo disco confirmou ATA primary-master, 2.097.152 setores, localização da partição DFS via GPT e início do shell.
- Dispositivo ATA anunciado como `/dev/hda`, syscall de enumeração e `/bin/distroinstall` na mídia live; duas confirmações antes de criar GPT/ESP FAT16 e reservar o DFS.
- Instalação de ponta a ponta validada no QEMU: o utilitário gravou o disco virtual, a GPT passou `sgdisk -v`, o RAMFS instalado correspondeu ao payload live e o disco iniciou até o shell sem a ISO.
- Scheduler cooperativo e programas separados para init/getty/shell.
- Sinais HUP, INT, KILL, SEGV, TERM, CONT e STOP, handlers/ignore para sinais
  termináveis, Ctrl+C e listagem de processos por snapshot via `/bin/ps`.
- RAMFS USTAR montado por path; `/sbin/init` inicia como PID 1, `/sbin/getty` inicia `/bin/shell`, e comandos externos são resolvidos em `/bin`.
- Built-ins `cd`, `pwd`, `echo`, `export`, `unset`, `set`, `read`, `clear`, `pid`, `type` e `exit`; parser com aspas, escapes e expansão simples de variáveis.
- Programas ELF externos em `/bin`, com `argv`/`envp`, busca por `PATH` e syscalls do RAMFS; init/getty e utilitários são fontes de userspace em `userland/`.
- Utilitários `dmesg` e `uname`; `uname` implementa seletores de nome, host,
  release, versão, arquitetura, sistema operacional e `-a`.
- Utilitário `shutdown` com mailbox IPC de pedido para PID 1; init continua
  supervisionando, o kernel sincroniza o ATA e solicita ACPI S5 usando
  GAS PM1 I/O/MMIO ou Sleep Control ACPI reduzido e AML `_S5_` descobertos via
  RSDP/FADT/DSDT.
- `globalconf` lê/atualiza `/home/.global/global.conf`; o kernel aplica layout US/ABNT2 durante a sessão e UTF-8 é o charset aceito.
- `vi` modal com navegação, edição UTF-8 e gravação de arquivos de até 4 KiB.

## Ainda ausente ou nao validado

- O scheduler não é preemptivo; faltam reclaim de frames, W^X, heap de userspace, drivers de rede e suporte completo a layouts de teclado. O armazenamento cobre apenas IDE primary-master PIO/LBA28 e GPT primária.
- Sinais são cooperativos, não possuem máscaras nem handlers aninhados.
  Falhas de CPU em ring 3 terminam o processo, mas ainda
  não são encaminhadas a um handler `SIGSEGV`. `ps` não é uma atualização
  contínua: não há `top` nem contabilidade de CPU.
- Um boot QEMU em imagem GPT confirmou montagem do DFS e início de `/sbin/init`, getty e shell; em uma repetição, o OVMF caiu no shell UEFI em vez de iniciar a imagem, então o boot pelo disco ainda precisa de uma verificação repetível. Não foi validada a persistência de uma escrita via shell nem a recuperação sob cortes de energia. O journal protege metadados, não dados dos arquivos, e pressupõe gravação atômica de setor e flush confiável. O importador USTAR não valida checksums nem cobre entradas especiais.
- O DFS tem 256 nós, caminhos de até 255 bytes, 14 extents por arquivo e remoção recursiva limitada a 32 níveis. Há suporte a uma conta adicional UID/GID 1000, sem grupos suplementares ou alteração de senha após a instalação. A senha root padrão continua `root`. Sem partição DFS válida, o fallback RAMFS é volátil e limitado a 128 nós/4 KiB por arquivo.
- `ls` ainda aceita somente um caminho e opções separadas; opções curtas combinadas não são suportadas. O shell ainda não tem pipelines, redirecionamento, aliases, funções ou `if/for/while`. UTF-8 é o único charset suportado e o mapa ABNT2 cobre apenas as teclas implementadas.

Os testes locais do DFS cobrem remount, metadados, ocultação, remoção recursiva,
checksums e replay de uma transação comprometida. Eles não substituem o teste
de boot e persistência em QEMU com uma imagem GPT.

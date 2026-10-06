# Arquitetura

O código-fonte do kernel está em `dnu/`; os programas de userspace ficam em
`userland/`. `Cargo.toml` aponta explicitamente para `dnu/main.rs`, enquanto o
`GNUmakefile` compila os programas e monta o arquivo USTAR distribuído pela
ISO.

## Boot

Limine carrega `.build/bootstrap.elf`, `boot/dzImage` e `boot/ramfs.tar`.
O bootstrap valida o payload LZ4 e seu CRC32, percorre os `PT_LOAD` do ELF64
do kernel, copia os segmentos/BSS para páginas físicas, adiciona mapeamentos
dos endereços virtuais do kernel às tabelas Limine e transfere um `BootInfo`
com respostas Limine e reservas físicas, incluindo todas as páginas da árvore
de page tables Limine ainda compartilhada pelos processos. O kernel é linkado em
`0xffffffff80000000`; o bootstrap reside em uma região virtual separada. O
kernel monta o USTAR diretamente da memória como raiz atual e carrega
monta o USTAR como initramfs e tenta montar o DFS persistente; se o DFS estiver
disponível, carrega `/sbin/init` da raiz persistente como PID 1 em ring 3.

As mensagens do kernel recebem um prefixo de tempo decorrido desde o entry
point, calibrado pelo PIT quando possível e convertido a partir do TSC. O
parser do RAMFS procura fontes em `system/fonts`; se não encontrar uma fonte
PSF válida, o kernel mantém a PSF1 8x16 `tools/fonts/zap-vga16.psf` embutida
no binário. Um `/sbin/init` ausente no RAMFS causa `KERNEL PANIC`.

## CPU e memória

`dnu/arch/x86_64/gdt.rs` instala GDT de kernel/user e TSS com stack ring 0 dedicada por processo. `dnu/arch/x86_64/idt.rs` instala gates para exceções 0 a 31 e um gate DPL3 em `int 0x80`; o handler registra vetor, error code, RIP, CS e CR2. O PIC legado é mascarado e IF fica desabilitado até haver remapeamento/controlador de interrupções.

`dnu/mm/memory.rs` percorre regiões `USABLE` do mapa Limine e oferece alocação monotônica de frames de 4 KiB pelo HHDM. `dnu/arch/x86_64/paging.rs` clona as mappings superiores do Limine e cria page tables de usuário independentes. `dnu/mm/heap.rs` fornece um bump allocator global de 1 MiB; `dealloc` é intencionalmente no-op.

`dnu/exec/elf.rs` valida ELF64 little-endian x86_64 ET_EXEC, limites da tabela de programas, segmentos `PT_LOAD` e entrypoint executável. `dnu/exec/process.rs` mapeia segmentos/BSS, `argv`, `envp` e stack em cada CR3; inicia ring 3 via `iretq` e executa novos binários do RAMFS. O frame `int 0x80` inclui I/O, `yield`, `exit`, PID, `clear`, `exec`, consulta de arquivos/diretórios, leitura de arquivo e listagem de diretório. Cópias entre user/kernel são traduzidas página a página.

`dnu/fs/ramfs.rs` monta o USTAR como initramfs/fallback e expõe paths,
leitura e listagem. Quando o DFS está montado, a fachada encaminha operações
de arquivos e diretórios ao filesystem persistente; sem DFS, usa um overlay
fixo de até 128 nós e 4 KiB por arquivo. As imagens executáveis são
construídas no staging do Make e empacotadas dentro de `ramfs.tar`, nunca
copiadas separadamente ao ESP. O shell tem built-ins de sessão, expande
variáveis simples e procura comandos externos nos diretórios de `PATH`.

`dnu/config.rs` carrega `/home/.global/global.conf` depois de montar as raízes
disponíveis. O charset aceito é UTF-8 e o layout de teclado (`us` ou `abnt2`)
é consultado durante o polling do PS/2. `globalconf` e `vi` gravam arquivos
de até 4 KiB; a gravação é persistente com DFS e volátil no fallback RAMFS.
A configuração padrão para novas imagens vem de
`ramfs/home/.global/global.conf`.

`dnu/drivers/block.rs` define a interface de setores de 512 bytes e
`dnu/drivers/ata.rs` implementa leitura/escrita PIO síncrona do master IDE
primário, limitada a LBA28. `dnu/fs/gpt.rs` valida a GPT primária, incluindo
checksums CRC32 do cabeçalho e do vetor de partições. No boot, o kernel tenta
montar e recuperar o DFS identificado pelo GUID reservado. `/dev/hda` é
listado dinamicamente. O `/bin/distroinstall` da mídia live recria a GPT,
grava uma ESP FAT16 e formata/semeia a partição DFS com a árvore USTAR
instalada. O utilitário host `tools/dfs-image.rs` faz a mesma formatação e
semeadura ao construir uma imagem de teste.
O RAMFS instalado omite o utilitário e os payloads de instalação.

## Console e entrada

`dnu/console/framebuffer.rs` escreve pixels RGB32, decodifica saída UTF-8 com
substituição para sequências inválidas e carrega a primeira fonte PSF1/PSF2 em
`ramfs/system/fonts`, usando a PSF1 8x16 empacotada no kernel como fallback.
Como leituras do filesystem são encaminhadas ao DFS quando montado, fontes
instaladas também podem ser encontradas nele.
O build inclui a fonte fallback em `system/fonts/zap-vga16.psf` no USTAR,
portanto ela também é semeada no DFS pelo instalador. `dnu/kernel_log.rs`
mantém as últimas 4 KiB de logs do kernel; syscall 25 e o utilitário `dmesg`
os expõem ao userspace.
`dnu/drivers/keyboard.rs` faz polling do controlador PS/2 set-1 e converte
teclas US ou ABNT2, incluindo acentos compostos e eventos de setas. A entrada
serial também decodifica UTF-8. COM1 continua disponível para diagnóstico e
terminal QEMU.

## Limites

O scheduler e cooperativo e nao ha timer/APIC ou preempcao. Todas as paginas user sao writable/executable; nao existe W^X, reclaim de frames, heap user ou validacao de checksum USTAR. Há uma fachada de filesystem com DFS persistente, mas ainda não um VFS genérico com múltiplos mounts e semântica POSIX completa.

O driver atual suporta apenas o master IDE primário via PIO e LBA28; não há enumeração PCI, AHCI, NVMe, VirtIO, interrupções de disco, cache ou concorrência de I/O. A GPT secundária não é usada como fallback. O ESP contém os arquivos de boot e o módulo RAMFS; a imagem de teste também tem uma partição DFS formatada e semeada.

O shell não implementa a gramática POSIX completa: sem pipes, redirecionamento, aliases, funções ou comandos compostos. `ls`, `cat`, `grep`, `mkdir`, `touch` e `rm` são ELFs externos; `grep` é literal e os leitores têm limite de 4 KiB. Os metadados DFS têm journal, mas os dados dos arquivos não são journaled; sem DFS, alterações ficam no overlay volátil. `cp` e `mv` ainda não existem.

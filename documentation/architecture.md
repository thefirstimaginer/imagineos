# Arquitetura

O código-fonte do kernel está em `dnu/`; os programas de userspace ficam em
`userland/`. `Cargo.toml` aponta explicitamente para `dnu/main.rs`, enquanto o
`GNUmakefile` compila os programas e monta o arquivo USTAR distribuído pela
ISO.

## Boot

Limine carrega `target/x86_64-unknown-none/release/dreamcore` e `boot/ramfs.tar`. O kernel é linkado em `0xffffffff80000000` com PADDRs a partir de 1 MiB, para satisfazer a regra do Limine contra PHDRs lower-half. O kernel monta o USTAR diretamente da memória e carrega `/sbin/init` como PID 1 em ring 3. O arquivo também contém `/sbin/getty`, `/bin/shell` e `/system/fonts`.

## CPU e memória

`dnu/arch/x86_64/gdt.rs` instala GDT de kernel/user e TSS com stack ring 0 dedicada por processo. `dnu/arch/x86_64/idt.rs` instala gates para exceções 0 a 31 e um gate DPL3 em `int 0x80`; o handler registra vetor, error code, RIP, CS e CR2. O PIC legado é mascarado e IF fica desabilitado até haver remapeamento/controlador de interrupções.

`dnu/mm/memory.rs` percorre regiões `USABLE` do mapa Limine e oferece alocação monotônica de frames de 4 KiB pelo HHDM. `dnu/arch/x86_64/paging.rs` clona as mappings superiores do Limine e cria page tables de usuário independentes. `dnu/mm/heap.rs` fornece um bump allocator global de 1 MiB; `dealloc` é intencionalmente no-op.

`dnu/exec/elf.rs` valida ELF64 little-endian x86_64 ET_EXEC, limites da tabela de programas, segmentos `PT_LOAD` e entrypoint executável. `dnu/exec/process.rs` mapeia segmentos/BSS, `argv`, `envp` e stack em cada CR3; inicia ring 3 via `iretq` e executa novos binários do RAMFS. O frame `int 0x80` inclui I/O, `yield`, `exit`, PID, `clear`, `exec`, consulta de arquivos/diretórios, leitura de arquivo e listagem de diretório. Cópias entre user/kernel são traduzidas página a página.

`dnu/fs/ramfs.rs` monta o USTAR e expõe paths, leitura e listagem de diretórios. Um overlay fixo de até 128 nós representa arquivos, diretórios e whiteouts mutáveis; `mkdir`, `touch`, `rm` e gravações de até 4 KiB alteram somente esse overlay em RAM, sem escrever no tar. As alterações desaparecem no reboot. As imagens executáveis são construídas no staging do Make e empacotadas dentro de `ramfs.tar`, nunca copiadas separadamente ao ESP. O shell tem built-ins de sessão, expande variáveis simples e procura comandos externos nos diretórios de `PATH`.

`dnu/config.rs` carrega `/home/.global/global.conf` depois de montar o RAMFS.
O charset aceito é UTF-8 e o layout de teclado (`us` ou `abnt2`) é consultado
durante o polling do PS/2. `globalconf` e `vi` gravam arquivos de até 4 KiB
somente no overlay de memória; a configuração embutida para builds é a cópia
em `ramfs/home/.global/global.conf`.

`dnu/drivers/block.rs` define a interface de setores de 512 bytes e
`dnu/drivers/ata.rs` implementa leitura/escrita PIO síncrona do master IDE
primário, limitada a LBA28. `dnu/fs/gpt.rs` valida a GPT primária, incluindo
checksums CRC32 do cabeçalho e do vetor de partições. No boot, a GPT é
inspecionada quando um disco ATA está presente; a partição com o GUID DFS é
reservada para o filesystem futuro. `/dev/hda` é listado dinamicamente. O
`/bin/distroinstall` da mídia live recria a GPT e grava uma ESP FAT16 com
Limine, kernel e o RAMFS de instalação; a partição GPT DFS permanece vazia.
O RAMFS instalado omite o utilitário e os payloads de instalação.

## Console e entrada

`dnu/console/framebuffer.rs` escreve pixels RGB32, inclui glifos de fallback
ASCII/português, decodifica saída UTF-8 com substituição para sequências
inválidas e carrega a primeira fonte PSF1/PSF2 em `ramfs/system/fonts`.
`dnu/drivers/keyboard.rs` faz polling do controlador PS/2 set-1 e converte
teclas US ou ABNT2, incluindo acentos compostos e eventos de setas. A entrada
serial também decodifica UTF-8. COM1 continua disponível para diagnóstico e
terminal QEMU.

## Limites

O scheduler e cooperativo e nao ha timer/APIC ou preempcao. Todas as paginas user sao writable/executable; nao existe W^X, reclaim de frames, heap user ou validacao de checksum USTAR. O sistema ainda nao tem VFS nem filesystem persistente: o driver de bloco e o leitor GPT sao apenas a base de armazenamento, e o kernel continua usando RAMFS USTAR como raiz.

O driver atual suporta apenas o master IDE primário via PIO e LBA28; não há enumeração PCI, AHCI, NVMe, VirtIO, interrupções de disco, cache ou concorrência de I/O. A GPT secundária não é usada como fallback. O ESP do disco de teste contém o kernel e o módulo RAMFS e pode inicializar pelo Limine, mas a partição DFS ainda não é formatada nem montada.

O shell não implementa a gramática POSIX completa: sem pipes, redirecionamento, aliases, funções ou comandos compostos. `ls`, `cat`, `grep`, `mkdir`, `touch` e `rm` são ELFs externos; `grep` é literal e os leitores têm limite de 4 KiB. O USTAR base é somente leitura; `mkdir`, `touch` e `rm` alteram um overlay volátil. `cp` e `mv` aguardam suporte a escrita persistente.

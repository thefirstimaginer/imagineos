# Arquitetura

## Boot

Limine carrega `target/x86_64-unknown-none/release/dreamcore` e `boot/ramfs.tar`. O kernel e linkado em `0xffffffff80000000` com PADDRs a partir de 1 MiB, para satisfazer a regra do Limine contra PHDRs lower-half. O kernel monta o USTAR diretamente da memoria; o RAMFS contem `/bin/init`, `/bin/getty`, `/bin/shell` e `/system/fonts`.

## CPU e memoria

`src/arch/x86_64/gdt.rs` instala GDT de kernel/user e TSS com stack ring 0 dedicada por processo. `src/arch/x86_64/idt.rs` instala gates para excecoes 0 a 31 e um gate DPL3 em `int 0x80`; o handler registra vetor, error code, RIP, CS e CR2. O PIC legado e mascarado e IF fica desabilitado ate haver remapeamento/controlador de interrupcoes.

`src/mm/memory.rs` percorre regioes `USABLE` do mapa Limine e oferece alocacao monotonica de frames de 4 KiB pelo HHDM. `src/arch/x86_64/paging.rs` clona as mappings superiores do Limine e cria page tables de usuario independentes. `src/mm/heap.rs` fornece um bump allocator global de 1 MiB; `dealloc` e intencionalmente no-op.

`src/exec/elf.rs` valida ELF64 little-endian x86_64 ET_EXEC, bounds da tabela de programas, segmentos `PT_LOAD` e entrypoint executavel. `src/exec/process.rs` mapeia segmentos/BSS, `argv`, `envp` e stack em cada CR3; inicia ring 3 via `iretq` (carregando RSP antes do seletor em AX) e executa novos binarios do RAMFS. O frame `int 0x80` inclui I/O, `yield`, `exit`, PID, `clear`, `exec`, consulta de arquivos/diretorios, leitura de arquivo e listagem de diretorio. Copias entre user/kernel sao traduzidas pagina a pagina.

`src/fs/ramfs.rs` monta o USTAR e expoe paths, leitura e listagem de diretorios. Um overlay fixo de ate 128 nos representa arquivos vazios, diretorios e whiteouts mutaveis; `mkdir`, `touch` e `rm` alteram somente esse overlay em RAM, sem escrever no tar. As alteracoes desaparecem no reboot. As imagens executaveis sao construidas no staging do Make e empacotadas dentro de `ramfs.tar`, nunca copiadas separadamente ao ESP. O shell tem built-ins de sessao, expande variaveis simples, e procura comandos externos nos diretorios de `PATH`.

## Console e entrada

`src/console/framebuffer.rs` escreve pixels RGB32, inclui fonte 5x7 de fallback e carrega a primeira fonte PSF1/PSF2 em `ramfs/system/fonts`. O cursor pisca via polling do TSC enquanto o syscall de leitura aguarda. `src/drivers/keyboard.rs` faz polling do controlador PS/2 set-1 e converte teclas US/AltGr em `char`; o shell serializa a entrada em UTF-8. COM1 continua disponivel para diagnostico e terminal QEMU.

## Limites

O scheduler e cooperativo e nao ha timer/APIC ou preempcao. Todas as paginas user sao writable/executable; nao existe W^X, reclaim de frames, heap user ou validacao de checksum USTAR. O sistema nao tem VFS nem armazenamento persistente. O RAMFS USTAR e tratado como artefato confiavel e somente leitura.

O shell nao implementa a gramatica POSIX completa: sem pipes, redirecionamento, aliases, funcoes ou comandos compostos. `ls`, `cat` e `grep` sao ELFs externos; `grep` e literal e os leitores tem limite de 4 KiB. O RAMFS e somente leitura, portanto `cp`, `mv` e `rm` aguardam suporte a escrita.

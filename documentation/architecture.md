# Arquitetura

## Boot

Limine carrega `target/x86_64-unknown-none/release/dreamcore` e `boot/initrd.tar`. O kernel e linkado em `0xffffffff80000000` com PADDRs a partir de 1 MiB, para satisfazer a regra do Limine contra PHDRs lower-half. O initrd USTAR contem `init.elf`, `getty.elf` e `shell.elf`.

## CPU e memoria

`src/gdt.rs` instala GDT de kernel/user e TSS com stack ring 0 dedicada por processo. `src/idt.rs` instala gates para excecoes 0 a 31 e um gate DPL3 em `int 0x80`. As excecoes sao fatais; interrupcoes externas permanecem desabilitadas.

`src/memory.rs` percorre regioes `USABLE` do mapa Limine e oferece alocacao monotonica de frames de 4 KiB pelo HHDM. `src/paging.rs` clona as mappings superiores do Limine e cria page tables de usuario independentes. `src/heap.rs` fornece um bump allocator global de 1 MiB; `dealloc` e intencionalmente no-op.

`src/elf.rs` valida ELF64 little-endian x86_64 ET_EXEC, bounds da tabela de programas, segmentos `PT_LOAD` e entrypoint executavel. `src/process.rs` mapeia segmentos/BSS e stack em cada CR3, inicia ring 3 via `iretq` e escalona cooperativamente em `yield`/`exit`. O frame `int 0x80` suporta `write`, `read`, `yield`, `exit`, `getpid` e `clear`; `write` traduz cada pagina do ponteiro user antes de copiar.

## Console e entrada

`src/framebuffer.rs` escreve pixels RGB32, inclui fonte 5x7 de fallback e carrega fontes PSF1/PSF2 com tabelas Unicode. `src/keyboard.rs` faz polling do controlador PS/2 set-1 e converte teclas US/AltGr em `char`; o shell serializa a entrada em UTF-8. COM1 continua disponivel para diagnostico e terminal QEMU.

## Limites

O scheduler e cooperativo e nao ha timer/APIC ou preempcao. Todas as paginas user sao writable/executable; nao existe W^X, reclaim de frames, heap user ou validacao de checksum USTAR. O sistema nao tem VFS nem armazenamento persistente. O initrd e tratado como artefato confiavel.

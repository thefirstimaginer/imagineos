# Arquitetura

## Boot

Limine carrega `target/x86_64-unknown-none/release/dreamcore` e `boot/initrd.tar`. O entrypoint Rust valida a revisao Limine e consome HHDM, mapa de memoria e framebuffer. O initrd USTAR contem `/init`, `/getty` e `/shell`; os comandos `echo` e `exec /...` encadeiam init ate o shell integrado ao kernel.

## CPU e memoria

`src/gdt.rs` instala GDT de kernel e TSS com stack ring 0 dedicada. `src/idt.rs` instala gates para excecoes 0 a 31; todas sao fatais e param a CPU com uma mensagem serial. Interrupcoes externas permanecem desabilitadas.

`src/memory.rs` percorre regioes `USABLE` do mapa Limine e oferece alocacao monotonica de frames de 4 KiB pelo HHDM. As tabelas de paging fornecidas pelo Limine permanecem ativas; nao ha page-table manager, heap, liberacao de frames ou isolamento entre processos.

## Console e entrada

`src/framebuffer.rs` escreve pixels RGB32, inclui fonte 5x7 de fallback e carrega fontes PSF1/PSF2 com tabelas Unicode. `src/keyboard.rs` faz polling do controlador PS/2 set-1 e converte teclas US/AltGr em `char`; o shell serializa a entrada em UTF-8. COM1 continua disponivel para diagnostico e terminal QEMU.

## Limites

O shell e uma tarefa foreground em ring 0, nao um programa ELF. Nao ha loader ELF, ring 3, syscalls, scheduler, timer/APIC, heap, VFS ou armazenamento persistente. O parser RAMFS le arquivos USTAR sem validar checksums; o initrd e um artefato confiavel construido junto da ISO.

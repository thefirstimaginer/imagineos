# Estado e limitacoes

## Implementado

- Entry point `no_std` x86_64 e requisicoes da crate `limine` 0.5.0.
- GDT de kernel, TSS com stack propria e IDT fatal para excecoes de CPU.
- Frame allocator monotonicamente crescente sobre regioes `USABLE` via HHDM.
- Console COM1, framebuffer RGB32, fonte 5x7 embutida e carregador PSF1/PSF2.
- Polling de teclado PS/2 set-1, incluindo algumas teclas AltGr convertidas em Unicode.
- Heap global bump de 1 MiB e page tables user derivadas das mappings Limine.
- Parser ELF64 x86_64, loader de `PT_LOAD`, zero de BSS e stacks user.
- GDT ring 3, gate `int 0x80`, syscalls de I/O, yield, exit, PID e clear.
- Scheduler cooperativo e ELFs separados para init/getty/shell.

## Ainda ausente ou nao validado

- A ISO anterior foi gerada antes destas implementacoes; precisa ser reconstruida. QEMU/OVMF nao estao disponiveis neste workspace para validar o novo ring 3.
- O scheduler nao e preemptivo; interrupcoes externas continuam desativadas.
- Sem reclaim de frames, W^X, heap user, drivers de disco/rede, filesystem persistente ou layout de teclado completo.
- Sem drivers de disco/rede, filesystem persistente ou layout de teclado completo.

O check Rust e os testes locais ELF/USTAR passam, mas isso nao substitui o teste de boot real.

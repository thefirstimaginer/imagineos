# Estado e limitacoes

## Implementado

- Entry point `no_std` x86_64 e requisicoes da crate `limine` 0.5.0.
- GDT de kernel, TSS com stack propria e IDT fatal para excecoes de CPU.
- Frame allocator monotonicamente crescente sobre regioes `USABLE` via HHDM.
- Console COM1, framebuffer RGB32, fonte 5x7 embutida e carregador PSF1/PSF2.
- Polling de teclado PS/2 set-1, incluindo algumas teclas AltGr convertidas em Unicode.
- Parser USTAR e scripts RAMFS `/init`, `/getty`, `/shell`.
- Shell interno com comandos basicos.

## Ainda ausente ou nao validado

- ISO UEFI gerada como `distro/dreamcore-2026-10-03-18-15-astrid.iso`; El Torito EFI, ESP e initrd foram inspecionados. O boot nao foi testado porque QEMU/OVMF nao estao disponiveis.
- `/init` e script interpretado no kernel; nao existe loader ELF nem processo em ring 3.
- Nao existe scheduler, syscalls, heap, paging proprio, liberacao de frames ou interrupcoes externas.
- Sem drivers de disco/rede, filesystem persistente ou layout de teclado completo.

O check Rust e os testes locais do parser passam, mas isso nao substitui o teste de boot real.

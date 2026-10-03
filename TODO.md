# TODO - ImagineOS Astrid

## Bring-up

- [x] Gerar ISO UEFI Limine.
- [ ] Validar boot em QEMU/OVMF.
- [ ] Configurar CI com target `x86_64-unknown-none` e teste USTAR.
- [ ] Testar respostas Limine ausentes e excecoes fatais.

## Memoria e processos

- [ ] Adicionar liberacao/coalescencia de frames e allocator concorrente.
- [ ] Gerenciar page tables e heap `alloc`.
- [ ] Implementar loader ELF64 e entrada ring 3.
- [ ] Definir syscalls, processos, scheduler e ciclo de vida de init.

## Dispositivos

- [ ] Implementar interrupcoes externas e timer APIC.
- [ ] Ampliar layout PS/2 e validar entrada UTF-8 em QEMU.
- [ ] Empacotar uma fonte PSF2 no initrd.
- [ ] Adicionar armazenamento e filesystem persistente.

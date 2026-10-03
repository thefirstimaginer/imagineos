# TODO - ImagineOS Astrid

## Bring-up

- [x] Gerar ISO UEFI Limine.
- [ ] Validar boot em QEMU/OVMF.
- [ ] Configurar CI com target `x86_64-unknown-none` e teste USTAR.
- [ ] Testar respostas Limine ausentes e excecoes fatais.

## Memoria e processos

- [ ] Adicionar liberacao/coalescencia de frames e allocator concorrente.
- [x] Page tables de usuario e heap bump de kernel (sem reclaim ainda).
- [x] Loader ELF64, entrada ring 3 e syscalls basicas.
- [x] Processos init/getty/shell com scheduler cooperativo.
- [ ] Adicionar liberacao/coalescencia de frames e allocator concorrente.
- [ ] Implementar preempcao por timer/APIC e heap reclaim.

## Dispositivos

- [ ] Implementar interrupcoes externas e timer APIC.
- [ ] Ampliar layout PS/2 e validar entrada UTF-8 em QEMU.
- [ ] Empacotar uma fonte PSF2 no initrd.
- [ ] Adicionar armazenamento e filesystem persistente.

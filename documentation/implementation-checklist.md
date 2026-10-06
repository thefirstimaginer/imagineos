# Checklist de implementacao

## Boot e kernel

- [x] Migrar o alvo principal para Rust `no_std`.
- [x] Solicitar mapa de memoria, HHDM, framebuffer e modulo pela crate Limine.
- [x] Instalar GDT/TSS e handlers fatais de excecao.
- [x] Inicializar allocator monotonicamente crescente de frames.
- [x] Gerar ISO UEFI Limine.
- [ ] Validar boot e ring 3 em QEMU/OVMF.

## Console e init

- [x] Driver COM1 e framebuffer RGB32.
- [x] Fonte PSF2 embutida no kernel como fallback e carregamento PSF1/PSF2 do RAMFS.
- [x] Entrada PS/2 com conversao UTF-8 basica.
- [x] Parser USTAR e cadeia de ELFs `/init -> /getty -> /shell`.
- [ ] Testar teclado e console visual no guest.

## Execucao de programas

- [x] Heap bump Rust com `alloc`.
- [x] Page-table manager e roots separados por processo.
- [x] Loader ELF64 e transicao para ring 3.
- [x] ABI de syscall e scheduler cooperativo.
- [ ] Preempcao por timer/APIC, reclaim e permissoes W^X.

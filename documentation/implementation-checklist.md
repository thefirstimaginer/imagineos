# Checklist de implementacao

## Boot e kernel

- [x] Migrar o alvo principal para Rust `no_std`.
- [x] Solicitar mapa de memoria, HHDM, framebuffer e modulo pela crate Limine.
- [x] Instalar GDT/TSS e handlers fatais de excecao.
- [x] Inicializar allocator monotonicamente crescente de frames.
- [ ] Validar a ISO UEFI no QEMU/OVMF.

## Console e init

- [x] Driver COM1 e framebuffer RGB32.
- [x] Fonte de fallback e carregamento PSF1/PSF2.
- [x] Entrada PS/2 com conversao UTF-8 basica.
- [x] Parser USTAR e cadeia de scripts `/init -> /getty -> /shell`.
- [ ] Testar teclado e console visual no guest.

## Execucao de programas

- [ ] Heap Rust com `alloc`.
- [ ] Page-table manager e isolamento de enderecos.
- [ ] Loader ELF64 e transicao para ring 3.
- [ ] ABI de syscall, scheduler e ciclo de vida de processos.

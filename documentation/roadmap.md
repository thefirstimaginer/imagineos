# Roadmap

1. Gerar e validar uma ISO UEFI Limine em QEMU/OVMF.
2. Adicionar testes de excecao e diagnostico de RIP, CR2 e CR3.
3. Implementar gerenciador de page tables e allocator com liberacao segura.
4. Criar heap Rust com `alloc` e sincronizacao.
5. Implementar loader ELF x86_64, transicao ring 3, syscalls e scheduler.
6. Transformar init/getty/shell em programas independentes sobre um ABI documentado.
7. Adicionar timer/APIC, filas de teclado, layout UTF-8 configuravel e armazenamento.

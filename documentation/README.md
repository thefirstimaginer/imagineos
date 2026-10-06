# ImagineOS

ImagineOS e um sistema operacional experimental x86_64 com kernel freestanding em Rust, inicializado pelo protocolo Limine. O userspace inicial executa ELFs em ring 3 com scheduler cooperativo.

## Conteudo

- [Arquitetura](architecture.md)
- [Fluxo de boot e processo init](boot-flow.md)
- [Build e execucao](build.md)
- [Criar programas para o userspace](userspace.md)
- [Comandos do shell](commands.md)
- [Testes](testing.md)
- [Armazenamento, GPT e DFS](storage.md)
- [Estado atual e limitacoes](status.md)
- [Roadmap](roadmap.md)

## Estado resumido

O sistema ja possui:

- boot x86_64 UEFI pelo Limine;
- requisicoes Limine para HHDM, mapa de memoria, framebuffer e modulo RAMFS;
- GDT/TSS, IDT fatal para excecoes de CPU e allocator de frames;
- RAMFS USTAR usado como initramfs/fallback, DFS persistente e `/init -> /getty -> /shell` em ring 3;
- descoberta do disco ATA em `/dev/hda`, GPT e instalador `/bin/distroinstall`;
- page tables de usuario, heap bump de kernel, syscalls `int 0x80` e scheduler cooperativo;
- logs do kernel com tempo decorrido, framebuffer com fonte PSF embutida de
  fallback e carregador PSF1/PSF2;
- `dzImage` LZ4 com bootstrap ELF Limine, verificação CRC32 e progresso de
  descompressão atualizado na mesma linha;
- teclado PS/2 por polling com conversao UTF-8 para algumas teclas AltGr;
- shell de usuario com `help`, `clear`, `pid`, `echo` e `exit`.
- built-ins de sessao e utilitarios externos `ls`, `cat`, `grep`, `mkdir`,
  `rm`, `touch`, `vi` e `globalconf` em `/bin`.

O DFS inclui journal redo de metadados, recuperação no mount, importação do
USTAR do sistema instalado e fallback para o RAMFS. Seu limite atual é de 256
nós e 14 extents por arquivo; conteúdo de arquivo não é journaled e permissões
ainda são apenas metadados. Timer/APIC, preempção, reclaim de frames, W^X,
drivers de armazenamento além de ATA PIO e enforcement de permissões ainda
faltam. O fluxo de boot e as limitações estão descritos em
[Fluxo de boot e processo init](boot-flow.md).

## Objetivo

A meta atual e validar e amadurecer o runtime inicial em QEMU: processos, paging, interrupcoes e ciclo de vida. Um display server ou driver grafico em userspace somente deve ser iniciado depois dessa base passar por testes repetidos sem reinicios.

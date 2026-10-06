# Fluxo de boot e processo init

## Fluxo atual do ImagineOS/DNU

```text
UEFI
  -> Limine
  -> bootstrap ELF + dzImage e ramfs.tar como módulos
  -> descompressão LZ4 e carga dos segmentos ELF do kernel
  -> repasse das respostas Limine e reservas de memória
  -> montagem do USTAR em memória como raiz atual
  -> /sbin/init (PID 1, ring 3)
  -> /sbin/getty
  -> /bin/shell
```

1. O firmware UEFI inicia o Limine. A configuração [`limine.conf`](../limine.conf)
   aponta para `bootstrap.elf` e carrega `dzImage` e `ramfs.tar` como módulos.
2. O bootstrap pede ao Limine o mapa de memória, HHDM, framebuffer e módulos.
   Ele verifica o cabeçalho dzImage, descomprime o ELF do kernel com LZ4,
   confere o CRC32 e atualiza `Unpacking Kernel - N%/100%` na mesma linha.
3. O bootstrap valida os segmentos ELF64 `PT_LOAD`, aloca páginas físicas,
   copia segmentos, zera BSS e mapeia os endereços virtuais do kernel nas
   tabelas Limine. Também reserva a árvore de page tables ativa do Limine:
   processos compartilham esses mapeamentos de kernel, então o frame allocator
   não pode reutilizar as páginas que guardam essas tabelas. Passa ao kernel o
   mapa, framebuffer, módulos e as regiões físicas reservadas.
4. O kernel inicializa as estruturas de CPU e memória, prepara o console com
   `tools/fonts/zap-vga16.psf` como fallback e monta o USTAR fornecido pelo
   Limine.
5. O USTAR serve como initramfs e também continua sendo a raiz atual: o kernel
   ainda não implementa DFS/VFS nem `switch_root`/`pivot_root`. A partição DFS
   pode ser identificada pela GPT, mas não é montada. A primeira fonte PSF1/PSF2
   de `system/fonts/` no RAMFS substitui a fonte de fallback; caso não exista,
   permanece a fonte 8x16 `tools/fonts/zap-vga16.psf`.
6. O kernel procura `/sbin/init` no arquivo montado e inicia esse ELF em ring 3
   com PID 1. `init` e `getty` são compilados dos fontes em `userland/`; o nome
   final dos arquivos no USTAR não tem sufixo `.elf`, embora seus conteúdos
   sejam executáveis ELF.
   Se `/sbin/init` não existir, o kernel para com uma mensagem `KERNEL PANIC`.
7. O programa `init` inicia `/sbin/getty`; getty inicia `/bin/shell`. Os
   utilitários e o shell ficam em `/bin`.

## Relação com o modelo Linux

O arquivo USTAR fornecido como módulo é semelhante, em propósito, a um
initramfs: permite que o kernel encontre o primeiro programa de userspace sem
precisar primeiro montar um disco. No DNU atual, porém, ele também é a única
árvore de arquivos disponível e permanece a raiz após o boot. A mensagem de log
de montagem do RAMFS descreve o estado real, não um placeholder. O disco só é
consultado para detectar ATA e GPT; até que DFS/VFS seja implementado, não há
montagem de uma raiz persistente nem operação `switch_root`/`pivot_root`.

As linhas de log do kernel começam com `[segundos.milissegundos]`, medidos a
partir do entry point por TSC calibrado pelo PIT quando possível. Saída de
programas userspace não recebe esses prefixos.

O início em ring 3 e o uso de um ELF para PID 1 seguem a mesma separação
fundamental entre kernel e userspace. O modelo de processos ainda é muito menor
que o Linux: a chamada `exec` do DNU cria outro processo e suspende o
chamador até esse processo terminar; ela não substitui a imagem do processo
chamador como `execve` faz no Linux. Também não há chamadas de `wait`, coleta
de processos filhos, reinício de serviços ou recuperação das páginas de um
processo encerrado. Portanto, quando a cadeia getty/shell termina, `init`
encerra e o kernel para ao não haver outro processo ativo. Esse fluxo é
intencionalmente inicial, não um gerenciador de serviços completo.

## Organização do repositório

- `dnu/`: fontes do kernel e seus subsistemas; o ponto de entrada é
  `dnu/main.rs`.
- `userland/`: fontes Rust dos programas executados em ring 3, incluindo
  `init.rs`, `getty.rs`, `shell.rs` e `utilities/`.
- `ramfs/`: arquivos de dados de origem, como fontes e diretórios iniciais.
  Durante o staging o build acrescenta os programas compilados em `/sbin` e
  `/bin`.
- `distro/`: imagens ISO geradas.
- `documentation/`: arquitetura, build, fluxo de boot e guias de contribuição.

Os manifests e as regras de build ficam na raiz: `Cargo.toml` define os bins
`dreamcore` e `bootstrap`, e `GNUmakefile` compila o kernel, cria `dzImage`,
compila o bootstrap e empacota os programas de `userland/` no USTAR.

## Recursos ainda não implementados

O dzImage é um formato próprio de payload LZ4 com tamanho e CRC32, não é
compatível com `vmlinuz` nem substitui o Limine. Ainda não há descoberta
abrangente de hardware por ACPI/Device Tree, KMS ou raiz persistente.

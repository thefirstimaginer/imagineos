# Armazenamento, GPT e DFS

## Camadas disponíveis

- `dnu/drivers/block.rs` define operações síncronas para setores de 512 bytes.
- `dnu/drivers/ata.rs` implementa PIO para o primary-master IDE, limitado a
  LBA28 (até 128 GiB). Ainda não há PCI, AHCI, NVMe ou VirtIO.
- `dnu/fs/gpt.rs` lê a GPT primária, valida CRC32 do cabeçalho e da tabela e
  localiza partições pelo GUID. O ImagineOS não interpreta MBR como tabela de
  partições nem usa a GPT secundária.
- `dnu/fs/dfs.rs` implementa o Dreamcore File System (DFS), incluindo formato,
  leitura/escrita, metadados, journal e recuperação.

O GUID de tipo reservado para a partição DFS é
`8a7f2c9d-6b31-4e52-9b14-445346530001`.

## DFS persistente

O instalador cria a GPT, formata a partição DFS e importa nela o arquivo USTAR
com o sistema instalado. No boot, o kernel monta primeiro o USTAR fornecido
pelo bootloader para ter um initramfs disponível; em seguida, tenta montar e
recuperar o DFS. Quando essa montagem funciona, as operações de arquivos e
diretórios são encaminhadas ao DFS e `/sbin/init` é carregado da raiz
persistente. Se não houver partição DFS ou a montagem/recovery falhar, o
RAMFS continua disponível como raiz de fallback.

O journal é do tipo redo para setores de metadados: grava payload e flush,
grava o registro de commit e flush, aplica as alterações e limpa o journal.
Uma transação comprometida que não tenha sido totalmente aplicada é repetida
no mount. O formato pressupõe gravações atômicas de setor e que `flush`
realmente persista as gravações.

Limites e ressalvas atuais:

- 256 nós no total; diretórios são representados por caminhos completos;
- caminho de até 255 bytes e até 14 extents por arquivo;
- até 16 setores de destino por transação; o journal reserva 17 setores
  (cabeçalho mais até 16 setores de payload);
- profundidade máxima de 32 níveis na remoção recursiva;
- importação USTAR aceita arquivos regulares e diretórios, mas ainda não
  valida o checksum do cabeçalho nem importa links, dispositivos ou outros
  tipos especiais;
- dados de arquivo não são journaled. Uma queda durante uma escrita pode
  deixar o conteúdo parcialmente atualizado, embora os metadados sejam
  recuperáveis;
- UID, GID e modo são armazenados e exibidos; o kernel aplica owner/group/other
  em operações de arquivo e diretório. O UID 0 ignora permissões; ainda não há
  grupos suplementares, ACLs ou gerenciamento de contas após a instalação;
- sem DFS montável, alterações no overlay do RAMFS são voláteis. O overlay
  continua limitado a 128 nós e arquivos de 4 KiB.

Todos os caminhos continuam acessíveis explicitamente, mas nomes de filhos
iniciados por `.` ficam fora das listagens padrão. A opção `ls -a` os revela.

## Imagem de disco virtual

`make disk-image` cria `.build/imagineos-disk.img` apenas se o caminho ainda
não existir. `tools/install-disk.sh` particiona uma imagem raw GPT de 1 GiB:

1. A partição 1 é uma ESP FAT16 de 128 MiB com Limine, bootstrap, `dzImage`,
   `ramfs.tar` e fallback UEFI.
2. A partição 2 ocupa o espaço restante e é formatada como DFS. O utilitário
   host `.build/dfs-image` importa nela a árvore USTAR do sistema instalado.

O protective MBR existe por exigência da GPT; o kernel só usa a GPT. `make
run-disk` inicia a imagem pelo QEMU/OVMF com IDE legado, sem ISO. O alvo não
altera discos físicos e recusa sobrescrever imagens existentes.

## Dispositivos e instalador

O ATA primary-master é anunciado como `/dev/hda` e listado por `ls /dev`.
`/bin/distroinstall`, disponível na mídia live, instala no disco primário.
Antes de gravar, exige `APAGAR /dev/hda` e depois `INSTALAR`. A instalação
recria a GPT e apaga os dados anteriores; a confirmação dupla não é uma
fronteira de segurança contra código kernel ou substituição do sistema.

Para testar sem disco físico, `make run-installer` cria
`.build/installer-target.img` se ela ainda não existir. Rode `distroinstall`
na mídia live e depois inicialize o alvo com
`make run-disk DISK_IMAGE=.build/installer-target.img`. Use
`OVMF_CODE=/usr/share/OVMF/OVMF_CODE_4M.fd` se esse for o caminho do firmware.

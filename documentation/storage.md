# Armazenamento, GPT e DFS

## Camadas disponíveis

- `dnu/drivers/block.rs` define operações síncronas de leitura/escrita de um
  setor de 512 bytes e enumera os erros de dispositivo.
- `dnu/drivers/ata.rs` implementa PIO para o master do canal IDE primário. A
  capacidade é limitada a LBA28 (até 128 GiB); não há detecção de PCI, AHCI,
  NVMe ou VirtIO.
- `dnu/fs/gpt.rs` lê a GPT primária no LBA 1, valida o CRC32 do cabeçalho e da
  tabela de entradas e consulta partições por GUID. Não há fallback para MBR
  nem leitura de GPT secundária.

O GUID de tipo reservado para a partição DFS é
`8a7f2c9d-6b31-4e52-9b14-445346530001`. O parser rejeita cabeçalhos e vetores
de partições com limites ou checksums inválidos.

## Imagem de disco de teste

`make disk-image` usa `tools/install-disk.sh` para criar
`.build/imagineos-disk.img`, uma imagem raw GPT de 1 GiB:

1. A partição 1 é uma ESP de 128 MiB, FAT16, com Limine, kernel, `ramfs.tar`
   e um `startup.nsh` de fallback para o shell UEFI. O kernel é iniciado por
   `bootstrap.elf`, que descompacta `dzImage`.
2. A partição 2 ocupa o espaço restante e usa o GUID reservado ao DFS; ela
   permanece vazia e sem formatação enquanto o filesystem não existir.

A imagem só é criada se o caminho de saída ainda não existir. `make run-disk`
inicia-a via OVMF, usando o chipset PC com IDE legado para permitir também o
teste do driver ATA PIO, sem a ISO. A GPT contém o protective MBR exigido pelo
formato; o ImagineOS não interpreta MBR como tabela de partições.

O boot pelo disco ainda usa `ramfs.tar` como raiz. O instalador prepara uma
imagem de teste inicializável; não é um instalador executado dentro do
ImagineOS nem uma instalação persistente do DFS.

Essa mensagem não é apenas um placeholder: o USTAR do Limine é montado e
permanece como a raiz usada para procurar `/sbin/init` e os arquivos dos
programas. Embora o kernel encontre a partição GPT reservada para DFS, não
existe ainda driver de filesystem que permita montá-la ou trocar a raiz. A
inicialização usa o RAMFS como initramfs disponível, mas não executa
`switch_root`/`pivot_root`; portanto, uma raiz persistente em disco depende da
implementação futura do DFS/VFS.

## Dispositivos e instalador do sistema

O disco ATA primary-master detectado aparece como `/dev/hda` e é listado por
`ls /dev`. Este é atualmente o único nome de dispositivo suportado; o nó serve
para descoberta e informação, enquanto o acesso bruto não é exposto como um FD
gravável.

O utilitário userspace `/bin/distroinstall` lista os discos e instala no
`/dev/hda`. Antes de escrever, exige que o usuário digite exatamente
`APAGAR /dev/hda` e depois `INSTALAR`; cancelar qualquer etapa não altera o
disco. A instalação recria a GPT, formata a ESP FAT16 e copia Limine,
`bootstrap.elf`, `dzImage`, `ramfs.tar` e os arquivos de fallback UEFI, deixando
o restante reservado ao DFS. Todo o conteúdo e a tabela de partições anteriores
são destruídos. O disco precisa ter pelo menos 132 MiB. O comando e os payloads
de instalação só são empacotados na mídia live; o RAMFS instalado não contém
`/bin/distroinstall` nem os payloads de origem.

Para experimentar sem um disco físico, `make run-installer` inicializa a ISO e
anexa `.build/installer-target.img` como alvo de 1 GiB. O utilitário pode então
instalar o sistema nessa imagem. `make run-disk DISK_IMAGE=.build/installer-target.img`
serve para testar o boot instalado. Use
`OVMF_CODE=/usr/share/OVMF/OVMF_CODE_4M.fd` se esse for o caminho do firmware.

A partição DFS permanece vazia; nesta versão, mesmo após a instalação, a raiz
segue sendo o RAMFS e as alterações de arquivos não sobrevivem ao reboot.
O kernel não expõe I/O de bloco genérico: oferece enumeração de discos,
capacidade e o serviço de instalação, restrito ao processo
`/bin/distroinstall` da mídia live. Como ainda não há um modelo geral de
permissões/capabilities, a confirmação dupla não é uma fronteira de segurança
contra código kernel ou substituição do sistema.

## Próximos passos para o DFS

Ainda precisam ser implementados o formato em disco (superbloco, alocação,
diretórios e arquivos), montagem e integração com o RAMFS/VFS e file
descriptors. A interface de bloco já foi usada para gravar a GPT e ESP durante
a instalação, mas não há filesystem persistente na partição DFS.

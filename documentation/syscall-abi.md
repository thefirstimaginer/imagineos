# ABI de syscalls do ImagineOS

Este documento define a ABI de userspace **versão 2**. Os números e contratos
abaixo são compartilhados pelo kernel e pela crate `imagineos` por meio de
[`shared/abi`](../shared/abi/src/lib.rs). Não reutilize números existentes com
novas semânticas. Mudanças incompatíveis exigem uma nova versão de ABI; novas
chamadas recebem novos números.

## Convenção de chamada

- Arquitetura: x86_64; entrada no kernel por `int 0x80`.
- `RAX` contém o número da chamada. Os argumentos, quando usados, ficam em
  `RDI`, `RSI`, `RDX`, `R10`, `R8` e `R9`, nessa ordem.
- O resultado retorna em `RAX`. Erros são inteiros negativos codificados como
  `-errno` em complemento de dois. A crate os converte em `Result<T, Error>`.
- Ponteiros e comprimentos são passados separadamente; strings de caminho e
  buffers de bytes não precisam de NUL terminal. Strings de argumentos e
  ambiente de `exec` são copiadas para o novo processo e terminadas pelo
  kernel; NUL interno não é aceito.
- `1` significa sucesso booleano e `0`, falso para `is_dir`/`is_file`. Essas
  duas chamadas retornam falso para caminho inexistente ou de tipo diferente;
  strings vazias, inválidas ou com NUL retornam `-EINVAL`.
- Chamadas não implementadas retornam `-ENOSYS` (`-38`).
- `exit` não retorna; o argumento de status ainda é ignorado pelo kernel.

## Chamadas

| Número | Nome | Argumentos | Resultado |
|---:|---|---|---|
| 1 | `write` | `RDI=bytes`, `RSI=tamanho` | bytes escritos, no máximo 512 por chamada |
| 2 | `read` | nenhum | escalar Unicode da entrada do console |
| 3 | `yield` | nenhum | `0` |
| 4 | `exit` | `RDI=status` (reservado; ignorado) | não retorna |
| 5 | `getpid` | nenhum | PID atual |
| 6 | `clear` | nenhum | `0` |
| 7 | `exec` | `RDI=caminho`, `RSI=tamanho`, `RDX=vetor UserArg argv`, `R10=argc`, `R8=vetor UserArg envp`, `R9=envc` | PID criado quando o chamador voltar a executar |
| 8 | `is_dir` | `RDI=caminho`, `RSI=tamanho` | `0` ou `1` |
| 9 | `is_file` | `RDI=caminho`, `RSI=tamanho` | `0` ou `1` |
| 10 | `read_file` | `RDI=caminho`, `RSI=tamanho`, `RDX=destino`, `R10=capacidade` | bytes lidos |
| 11 | `readdir` | `RDI=caminho`, `RSI=tamanho`, `RDX=destino`, `R10=capacidade` | bytes escritos no destino |
| 12 | `mkdir` | `RDI=caminho`, `RSI=tamanho`, `RDX=criar pais` | `0` |
| 13 | `touch` | `RDI=caminho`, `RSI=tamanho` | `0` |
| 14 | `remove` | `RDI=caminho`, `RSI=tamanho`, `RDX=recursivo` | `0` |
| 15 | `write_file` | `RDI=caminho`, `RSI=tamanho do caminho`, `RDX=conteúdo`, `R10=tamanho` | bytes gravados |
| 16 | `abi_version` | nenhum | versão ABI (`2`) |
| 17 | `open` | `RDI=caminho`, `RSI=tamanho`, `RDX=flags` | descritor aberto |
| 18 | `read_fd` | `RDI=fd`, `RSI=destino`, `RDX=capacidade` | bytes lidos; `0` no EOF |
| 19 | `write_fd` | `RDI=fd`, `RSI=bytes`, `RDX=tamanho` | bytes escritos |
| 20 | `close` | `RDI=fd` | `0` |
| 21 | `disk_count` | nenhum | quantidade de discos de bloco detectados |
| 22 | `disk_sectors` | `RDI=índice do disco` | setores endereçáveis; `-ENODEV` se não existir |
| 23 | `install_disk` | `RDI=índice do disco` | `0` após instalação inicializável; operação destrutiva |
| 24 | `stat` | `RDI=caminho`, `RSI=tamanho`, `RDX=destino UserStat` | `0` |
| 25 | `dmesg` | `RDI=destino`, `RSI=capacidade` | bytes da parte mais recente do log circular do kernel |
| 26 | `shutdown_request` | nenhum | UID 0: `0` e posta um pedido; outros recebem `-EPERM` |
| 27 | `shutdown_poll` | nenhum | PID 1: `1` se havia pedido, `0` caso contrário; outros recebem `-EPERM` |
| 28 | `power_off` | nenhum | PID 1 sincroniza o disco e solicita desligamento; outros recebem `-EPERM` |
| 29 | `kill` | `RDI=PID`, `RSI=sinal` | `0`; `-EINVAL` para sinal desconhecido, `-ESRCH` para PID inexistente, `-EPERM` sem permissão sobre o alvo |
| 30 | `sigaction` | `RDI=sinal`, `RSI=handler`, `RDX=restorer` | `0`; handler `0` restaura ação padrão, `1` ignora, endereço maior que `1` registra handler |
| 31 | `sigreturn` | nenhum | restaura o contexto salvo ao retornar de um handler |
| 32 | `process_list` | `RDI=destino ProcessInfo[]`, `RSI=capacidade em itens` | número de processos copiados (máximo 8) |
| 33 | `authenticate` | `RDI=usuário`, `RSI=tamanho`, `RDX=senha`, `R10=tamanho`, `R8=UID solicitado` (`0xffffffff` seleciona UID da conta) | `0` e troca da identidade autenticada; `-EPERM` se falhar |
| 34 | `getidentity` | `RDI=destino UserIdentity` | `0` e copia UID/GID/flag administrativa/nome/hostname |
| 35 | `install_disk_config` | `RDI=índice`, `RSI=ponteiro InstallConfig` | `0` após instalação e gravação das configurações no DFS |

`install_disk` e `install_disk_config` aceitam apenas o processo
`/bin/distroinstall` incluído na mídia live e UID 0; o RAMFS instalado omite o
utilitário e seus payloads. Discos são anunciados para descoberta, não como FDs
de bloco graváveis. `InstallConfig` tem comprimentos fixos para nome (32 bytes),
senha (64 bytes) e hostname (64 bytes), além de flags `add_user` e
`administrator`. O instalador valida o contrato, prepara o hash e exige RDRAND
antes de apagar o disco. A configuração é salva em `/etc/hostname` e, se
solicitado, `/etc/users.db`.

`UserArg` é uma estrutura `#[repr(C)]` composta por dois `u64`: endereço e
comprimento. `exec` aceita até 12 argumentos e 12 entradas de ambiente, cada
uma com até 128 bytes; o caminho também tem limite de 128 bytes. Consultas de
arquivo/diretório aceitam caminhos de até 128 bytes e buffers de até 4 KiB.
Operações mutáveis de RAMFS aceitam caminhos de até 256 bytes; `write_file`
aceita até 4 KiB por arquivo. Valores além desses limites são rejeitados.
Cada processo tem 16 slots de descritor: `0` stdin, `1` stdout, `2` stderr e
13 descritores adicionais. A tabela é copiada para processos iniciados por
`exec`; descritores de arquivo herdam caminho, flags e offset por cópia, não
como uma descrição de arquivo compartilhada. Stdin lê caracteres Unicode do
console e os entrega em UTF-8; stdout/stderr escrevem no console/framebuffer.
Os arquivos abertos são arquivos do RAMFS/DFS; no fallback RAMFS a escrita faz
copy-up para o overlay volátil e cada arquivo continua limitado a 4 KiB.

Flags aceitas por `open`: `OPEN_READ=1`, `OPEN_WRITE=2`, `OPEN_CREATE=4`,
`OPEN_TRUNCATE=8` e `OPEN_APPEND=16`. É necessário solicitar leitura ou escrita;
truncate e append exigem escrita e não podem ser combinados. O kernel ainda
não oferece diretórios como streams, seek, pipes, sockets, dispositivos por
FD ou compartilhamento atômico de offsets entre processos. Leitura, escrita,
listagem e execução verificam owner/group/other com UID/GID; UID 0 é o único
bypass. O sinal só pode ser enviado ao próprio UID ou por UID 0.

Erros usados atualmente incluem `EPERM=1`, `ENOENT=2`, `ESRCH=3`, `EFAULT=14`, `ENODEV=19`, `EINVAL=22`,
`E2BIG=7`, `ENOEXEC=8`, `EAGAIN=11`, `ENOSPC=28`, `ENOSYS=38`,
`ENOTDIR=20`, `EISDIR=21`, `EEXIST=17`, `EMFILE=24`, `EBADF=9`, `EIO=5`,
`ENOTEMPTY=39` e `EOVERFLOW=75`. A crate preserva errno
desconhecidos em `Error::code()`.
`exec` retorna `ENOEXEC` quando o ELF não pode ser carregado; `EOVERFLOW`
indica que a leitura ou listagem não coube no buffer informado.

`UserStat` é uma estrutura `#[repr(C)]` com `size: u64`, `mode`, `uid`, `gid`
e `kind` como `u32`. `kind=1` identifica diretório e `kind=0`, arquivo.
`dmesg` copia no máximo 4 KiB do buffer circular do kernel, em ordem
cronológica; se o buffer do userspace for menor, recebe apenas a parte final.
O buffer mantém os últimos 4 KiB de mensagens enviadas pelo logger do kernel.
As syscalls 26–28 implementam um mailbox IPC limitado a pedidos de
desligamento: solicitações repetidas antes da leitura são agrupadas. Não é um
sistema de mensagens genérico. Somente UID 0 pode pedir o desligamento; PID 1
pode consumir o pedido ou iniciar o poweroff.
Antes do poweroff, o kernel executa `flush` no disco ATA detectado. A rotina
usa o RSDP do Limine para encontrar o FADT, os registradores PM1 e o estado S5
`_S5_` na DSDT; formatos AML ou registradores que não forem reconhecidos fazem
a chamada falhar em vez de presumir uma porta específica de QEMU.

Os sinais implementados são `SIGHUP=1`, `SIGINT=2`, `SIGKILL=9`,
`SIGSEGV=11`, `SIGTERM=15`, `SIGCONT=18` e `SIGSTOP=19`. `SIGKILL` encerra e
`SIGSTOP` suspende sem aceitar handlers ou ignorá-los; `SIGCONT` retoma um
processo suspenso. HUP, INT, SEGV e TERM terminam o processo por padrão, mas
podem ser ignorados ou capturados. Um handler recebe o número do sinal em
`RDI`; ao retornar, o stub de sinal da API executa `sigreturn`. Sinais ficam
pendentes até o processo voltar a uma syscall, pois o scheduler atual é
cooperativo. Não há máscaras, filas de ocorrências repetidas ou handlers
aninhados. PID 1 é protegido e não pode receber
sinais via `kill`. Exceções originadas em ring 3 encerram apenas o processo
afetado; elas ainda não são entregues a um handler `SIGSEGV`.

`ProcessInfo` é uma estrutura `#[repr(C)]` com PID, estado numérico, máscara de
sinais pendentes e nome do executável em um campo fixo de 64 bytes. A syscall
de listagem retorna um snapshot, limitado aos oito slots do scheduler; não
fornece uso de CPU, estados de serviço ou atualização contínua.

## Crate Rust de userspace

O crate [`imagineos`](../userland/api/src/lib.rs) é `no_std`, depende do crate
compartilhado do ABI e fornece wrappers com erros tipados. Exemplos:

```rust,ignore
imagineos::console::write_all(b"Olá, ImagineOS!\n")?;

let mut contents = [0; 4096];
let length = imagineos::fs::read_file("/home/readme.txt", &mut contents)?;

let pid = imagineos::process::exec(
    "/bin/hello",
    &["hello"],
    &[],
)?;
```

Os utilitários existentes são construídos contra essa crate por `make
user-programs`; `userland/utilities/common.rs` preserva uma fachada de transição
para as chamadas antigas. A crate separada `imagineos_rt` oferece macros
`main!`, `print!` e `println!`: o ponto de entrada gerado valida a ABI, chama a
função principal e converte seu retorno em código de saída; a formatação usa
`core::fmt`, sem alocação. `main!(with_args entry)` também converte os vetores
crus de argumentos e ambiente em `Arguments` e `Environment`. As macros de
impressão retornam `imagineos::Result<()>`, permitindo propagar erros com `?`.
O runtime instala um panic handler que encerra o processo com status 127 e
depende da crate `imagineos` para os wrappers de syscalls.
Utilitários `hello` e `fdtest` exercitam a API; `hello` usa o novo runtime. A
crate não implementa `std`, alocação dinâmica, a semântica completa de
descritores POSIX, threads ou persistência.

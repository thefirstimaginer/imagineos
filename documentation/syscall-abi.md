# ABI de syscalls do ImagineOS

Este documento define a ABI de userspace **versão 1**. Os números e contratos
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
- `exit` não retorna; o argumento de status da versão 1 é ignorado.

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
| 16 | `abi_version` | nenhum | versão ABI (`1`) |
| 17 | `open` | `RDI=caminho`, `RSI=tamanho`, `RDX=flags` | descritor aberto |
| 18 | `read_fd` | `RDI=fd`, `RSI=destino`, `RDX=capacidade` | bytes lidos; `0` no EOF |
| 19 | `write_fd` | `RDI=fd`, `RSI=bytes`, `RDX=tamanho` | bytes escritos |
| 20 | `close` | `RDI=fd` | `0` |
| 21 | `disk_count` | nenhum | quantidade de discos de bloco detectados |
| 22 | `disk_sectors` | `RDI=índice do disco` | setores endereçáveis; `-ENODEV` se não existir |
| 23 | `install_disk` | `RDI=índice do disco` | `0` após instalação inicializável; operação destrutiva |

`install_disk` aceita apenas o processo `/bin/distroinstall` incluído na mídia
live; o RAMFS instalado omite o utilitário e seus payloads. Discos são
anunciados para descoberta, não como FDs de bloco graváveis. A confirmação
dupla é feita pelo utilitário e não substitui permissões/capabilities gerais
de sistema, que ainda não existem.

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
Os arquivos abertos são arquivos do RAMFS; escrita faz copy-up para o overlay
volátil e cada arquivo continua limitado a 4 KiB.

Flags aceitas por `open`: `OPEN_READ=1`, `OPEN_WRITE=2`, `OPEN_CREATE=4`,
`OPEN_TRUNCATE=8` e `OPEN_APPEND=16`. É necessário solicitar leitura ou escrita;
truncate e append exigem escrita e não podem ser combinados. O kernel ainda
não oferece diretórios como streams, seek, pipes, sockets, dispositivos por
FD, permissões ou compartilhamento atômico de offsets entre processos.

Erros usados atualmente incluem `EPERM=1`, `ENODEV=19`, `EINVAL=22`, `EFAULT=14`, `ENOENT=2`,
`E2BIG=7`, `ENOEXEC=8`, `EAGAIN=11`, `ENOSPC=28`, `ENOSYS=38`,
`ENOTDIR=20`, `EISDIR=21`, `EEXIST=17`, `EMFILE=24`, `EBADF=9`,
`ENOTEMPTY=39` e `EOVERFLOW=75`. A crate preserva errno
desconhecidos em `Error::code()`.
`exec` retorna `ENOEXEC` quando o ELF não pode ser carregado; `EOVERFLOW`
indica que a leitura ou listagem não coube no buffer informado.

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
para as chamadas antigas. Utilitários `hello` e `fdtest` exercitam diretamente
a nova API. A crate não implementa `std`, alocação dinâmica, a semântica
completa de descritores POSIX, threads ou persistência.

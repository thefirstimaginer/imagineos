# Criar programas para o ImagineOS/Dreamcore (DNU)

O userspace atual executa programas ELF64 x86_64 em ring 3. O fluxo
documentado e suportado pelo repositório é escrever o programa em Rust sem
`std`, compilá-lo para `x86_64-unknown-none` e incluí-lo no RAMFS distribuído
com a imagem.

## Exemplo: criar um comando `hello`

Crie `userland/utilities/hello.rs`:

```rust
#![no_std]
#![no_main]

#[allow(dead_code)]
mod common;

#[no_mangle]
extern "C" fn _start(
    _argc: usize,
    _argv: *const *const u8,
    _envc: usize,
    _envp: *const *const u8,
) -> ! {
    common::write(b"Hello from ImagineOS!\n");
    common::exit(0)
}
```

Os utilitários em `userland/utilities/` compartilham `common.rs`, que fornece
wrappers para escrever no console, encerrar o processo e acessar algumas
operações do RAMFS. O ponto de entrada é `_start`, não `main`; o kernel chama
essa função com `argc`, `argv`, `envc` e `envp`.

Adicione `hello` à variável `USER_UTILITIES` no `GNUmakefile`, por exemplo:

```make
USER_UTILITIES := cat grep hello ls mkdir rm touch
```

Na raiz do repositório, compile e empacote o programa:

```sh
mkdir -p ramfs
make user-programs
make .build/ramfs.tar
tar -tf .build/ramfs.tar
```

O Makefile compila o fonte para um ELF em `.build/user/utilities/hello`
e o inclui como `bin/hello` dentro de `.build/ramfs.tar`. Não é necessário
copiar o executável manualmente para `ramfs/`: os ELFs dos programas listados
em `USER_UTILITIES` são adicionados durante o staging do build.

Para apenas gerar a ISO:

```sh
make iso
```

Para gerar a ISO e inicializá-la no QEMU/OVMF, use `make run` (esse alvo
executa `make iso` como pré-requisito):

```sh
make run
```

No shell do ImagineOS, execute `hello`. O shell procura programas externos nos
diretórios de `PATH`, cujo valor inicial é `/bin`. Os pré-requisitos de
ferramentas e firmware estão em [Build e execução](build.md); testes manuais
estão em [Testes](testing.md).

## Entrada, argumentos e syscalls

- O kernel inicia o programa em `_start` passando quatro parâmetros: contagem
  e vetor de argumentos, seguidos da contagem e vetor do ambiente. Argumentos
  e entradas de ambiente são strings terminadas em NUL; a contagem informa
  quantos itens há em cada vetor.
- Para utilitários, importe `common` como no exemplo. A implementação atual
  oferece `argument`, `environment_value`, `resolve_path`, `write`,
  `read_file`, `list_directory`, `mkdir`, `touch`, `remove` e `exit`.
- As chamadas de sistema usam `int 0x80`: número em `RAX`; argumentos em
  `RDI`, `RSI`, `RDX`, `R10`, `R8` e `R9`, nessa ordem. O ABI é específico do
  ImagineOS e ainda não é uma interface estável para terceiros.
- Os números implementados atualmente são: `1` escrever; `2` ler um caractere
  da entrada do console; `3` ceder a execução; `4` encerrar; `5` obter PID;
  `6` limpar o console; `7` iniciar ELF; `8` verificar diretório; `9` verificar
  arquivo; `10` ler arquivo; `11` listar diretório; `12` criar diretório;
  `13` criar arquivo vazio; `14` remover arquivo ou diretório.
- Os wrappers de `common.rs` tratam apenas parte dessas chamadas. Para usar
  outra chamada, consulte a implementação atual em
  [`dnu/abi/syscall.rs`](../dnu/abi/syscall.rs) e siga os wrappers existentes
  antes de adicionar código assembly próprio.

## Limitações atuais

- **Formato e linguagem:** o loader aceita ELF64 little-endian, arquitetura
  x86_64 e executáveis estáticos `ET_EXEC`. O fluxo do Makefile compila Rust
  `no_std`; não há biblioteca padrão, libc, linker dinâmico, PIE nem suporte
  documentado a toolchains de outras linguagens.
- **Entrada e execução:** o programa não recebe stdin/stdout/stderr como
  descritores de arquivo. Há chamadas para console, leitura de caractere,
  yield, exit e criação de processos, mas não há pipes, redirecionamento,
  sinais, espera/coleta de processo ou um contrato POSIX. O shell também não
  implementa pipes nem redirecionamento.
- **Recursos de processo:** há no máximo quatro slots de processo, incluindo
  processos ativos; o scheduler é cooperativo, sem preempção por timer. Um
  programa que não cede a execução pode impedir que outros avancem.
- **Memória e segurança:** não há heap de userspace fornecido pelo sistema.
  Programas podem usar buffers estáticos ou implementar seu próprio
  gerenciamento de memória, mas não devem presumir alocação dinâmica. As
  páginas de userspace não têm proteção W^X e não há recuperação de frames
  após o término de um processo.
- **Limites de ABI:** `exec` aceita até 12 argumentos e 12 entradas de
  ambiente, com até 128 bytes por item e caminho. A stack de userspace tem
  32 KiB. Escritas ao console são limitadas a 512 bytes por chamada; leitura
  de arquivo e listagem de diretório têm buffers de até 4 KiB. Wrappers de
  utilitários também podem impor limites menores.
- **Arquivos e persistência:** o conteúdo base do RAMFS é um arquivo USTAR
  somente de leitura. `mkdir`, `touch` e `remove` alteram apenas um overlay
  volátil em memória; não há armazenamento persistente nem acesso a disco ou
  rede. Alterações desaparecem ao reiniciar.
- **Estado de validação:** a implementação é experimental. Compilar e
  empacotar um programa não comprova que ele funciona no boot real; teste a
  ISO em QEMU/OVMF e verifique o comportamento no shell.

Essas limitações descrevem o runtime atual e podem mudar conforme o ABI e o
sistema evoluírem. Confira também [Estado atual e limitações](status.md) e
[Arquitetura](architecture.md).

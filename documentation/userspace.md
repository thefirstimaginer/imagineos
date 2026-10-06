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

#[no_mangle]
extern "C" fn _start(
    _argc: usize,
    _argv: *const *const u8,
    _envc: usize,
    _envp: *const *const u8,
) -> ! {
    if imagineos::syscall::abi_version() != Ok(imagineos::abi::ABI_VERSION) {
        imagineos::process::exit(2);
    }
    if imagineos::console::write_all(b"Hello from ImagineOS!\n").is_err() {
        imagineos::process::exit(1);
    }
    imagineos::process::exit(0)
}

#[panic_handler]
fn panic(_info: &core::panic::PanicInfo<'_>) -> ! {
    imagineos::process::exit(127)
}
```

O crate reutilizável fica em `userland/api/` e é ligado automaticamente pelo
Makefile. O ponto de entrada é `_start`, não `main`; o kernel chama essa função
com `argc`, `argv`, `envc` e `envp`. `userland/utilities/common.rs` permanece
como fachada de compatibilidade enquanto utilitários mais antigos migram para
a API tipada `imagineos`.

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
- Para utilitários existentes, `common` preserva uma fachada temporária. Novos
  programas devem depender da crate `imagineos`, que oferece módulos `console`,
  `fs`, `process` e `args`, além dos tipos `Result` e `Error`. A API atual
  oferece `console::write_all`, `console::read_char`, `fs::read_file`,
  `fs::write_file`, `fs::list_directory`, operações de diretório e arquivos,
  `fs::open` com `OpenOptions`, leitura/escrita/fechamento de descritores,
  stdin/stdout/stderr, `process::pid`, `process::yield_now`, `process::exec` e
  `process::exit`.
- A implementação de transição atual
  oferece `argument`, `environment_value`, `resolve_path`, `write`,
  `read_char`, `clear`, `read_file`, `write_file`, `list_directory`, `mkdir`,
  `touch`, `remove` e `exit`.
- As chamadas de sistema usam `int 0x80`: número em `RAX`; argumentos em
  `RDI`, `RSI`, `RDX`, `R10`, `R8` e `R9`, nessa ordem. Esse contrato é a ABI
  v1 do ImagineOS; números e semânticas não devem ser reutilizados ou alterados.
  O kernel e as aplicações compartilham as definições em
  [`shared/abi`](../shared/abi/src/lib.rs). A syscall `16` consulta a versão
  para que programas possam detectar incompatibilidade.
- A lista completa de chamadas, registradores, resultados, erros e limites está
  em [ABI de syscalls](syscall-abi.md). Não adicione assembly de syscall em
  aplicações; estenda a crate `imagineos` e mantenha o contrato documentado.

## Limitações atuais

- **Formato e linguagem:** o loader aceita ELF64 little-endian, arquitetura
  x86_64 e executáveis estáticos `ET_EXEC`. O fluxo do Makefile compila Rust
  `no_std` usando a crate `imagineos`; não há biblioteca padrão, libc, linker
  dinâmico, PIE nem suporte documentado a toolchains de outras linguagens.
- **Entrada e execução:** stdin/stdout/stderr e arquivos do RAMFS agora podem
  ser acessados por descritores. Ainda não há pipes, redirecionamento, sockets,
  descritores de dispositivos, seek, sinais, espera/coleta de processo ou um
  contrato POSIX. O shell também não implementa pipes nem redirecionamento.
- **Recursos de processo:** há no máximo quatro slots de processo, incluindo
  processos ativos; o scheduler é cooperativo, sem preempção por timer. Um
  programa que não cede a execução pode impedir que outros avancem.
- **Memória e segurança:** não há heap de userspace fornecido pelo sistema.
  Programas podem usar buffers estáticos ou implementar seu próprio
  gerenciamento de memória, mas não devem presumir alocação dinâmica. As
  páginas de userspace não têm proteção W^X e não há recuperação de frames
  após o término de um processo.
- **Limites de ABI:** cada processo tem 16 descritores, dos quais 13 podem ser
  abertos pelo aplicativo. A tabela é copiada em `exec`, inclusive offsets de
  arquivos, e descritores não compartilham offsets entre processos. `exec`
  aceita até 12 argumentos e 12 entradas de
  ambiente, com até 128 bytes por item e caminho. A stack de userspace tem
  32 KiB. Escritas ao console são limitadas a 512 bytes por chamada; leitura
  de arquivo e listagem de diretório têm buffers de até 4 KiB. Wrappers de
  utilitários também podem impor limites menores.
- **Arquivos e persistência:** o conteúdo base do RAMFS é um arquivo USTAR
  somente de leitura. `mkdir`, `touch`, `remove` e gravações de até 4 KiB
  alteram apenas um overlay volátil em memória. Arquivos USTAR abertos para
  escrita são copiados para esse overlay; cada arquivo continua limitado a
  4 KiB. Não há armazenamento persistente nem acesso a disco ou rede.
  Alterações desaparecem ao reiniciar.
- **Estado de validação:** a implementação é experimental. Compilar e
  empacotar um programa não comprova que ele funciona no boot real; teste a
  ISO em QEMU/OVMF e verifique o comportamento no shell.

Essas limitações descrevem o runtime atual e podem mudar conforme o ABI e o
sistema evoluírem. Confira também [Estado atual e limitações](status.md) e
[Arquitetura](architecture.md).

## Configuração global, teclado e charset

O arquivo inicial `ramfs/home/.global/global.conf` é incluído no USTAR como
`/home/.global/global.conf`. O kernel lê esse arquivo durante o boot:

```ini
charset=utf-8
keyboard=abnt2
```

UTF-8 é o único charset implementado. O driver de teclado usa o layout
selecionado pelo arquivo; `abnt2` inclui `ç/Ç`, AltGr, teclas estendidas e
composição básica de acentos (`´`, `` ` ``, `~`, `^` e `¨`). O layout `us`
também está disponível. O shell recebe valores Unicode e grava texto em UTF-8.

`globalconf set KEY VALUE` atualiza a cópia volátil do arquivo e aplica a
configuração válida imediatamente. Essa alteração não sobrevive ao reboot.
Para definir valores padrão que persistam entre builds, edite o arquivo fonte
em `ramfs/home/.global/global.conf` antes de compilar. O framebuffer usa o mapa
Unicode da fonte PSF carregada; se um caractere não existir, tenta desenhar o
glifo `?`. A fonte de fallback do kernel é
`tools/fonts/zap-vga32.psf`; fontes residentes no disco só poderão ser usadas
quando houver suporte a um filesystem persistente.

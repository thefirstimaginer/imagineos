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

fn user_main(
    mut args: imagineos_rt::Arguments<'_>,
    _env: imagineos_rt::Environment<'_>,
) -> imagineos_rt::imagineos::Result<()> {
    let name = match args.next() {
        Some(name) => name?,
        None => "ImagineOS",
    };
    imagineos_rt::println!("Hello, {name}!")?;
    Ok(())
}

imagineos_rt::main!(with_args user_main);
```

As bibliotecas ficam separadas em `userland/api/` e `userland/runtime/` e são
ligadas automaticamente pelo Makefile. A crate `imagineos` oferece wrappers de
syscalls; `imagineos_rt` depende dela e fornece o ciclo de vida de programas.
`imagineos_rt::main!` gera `_start`, valida a versão da ABI, converte o
resultado de `user_main` em status de saída e instala um panic handler. A macro
`imagineos_rt::println!` usa `core::fmt` e retorna
`imagineos_rt::imagineos::Result<()>`; o `?`
preserva erros de escrita em vez de ignorá-los. `userland/utilities/common.rs`
permanece como fachada de compatibilidade para utilitários antigos.

Adicione `hello` à variável `USER_UTILITIES` no `Makefile`, por exemplo:

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
  stdin/stdout/stderr, `fs::install_to_disk`, `kernel_log::read`,
  `process::pid`, `process::yield_now`, `process::exec` e `process::exit`.
- `users::identity` consulta UID, GID, nome, hostname e flag administrativa;
  `users::authenticate` valida a credencial e troca a identidade do processo
  apenas após validação no kernel. `fs::install_to_disk_with_config` envia os
  dados iniciais de conta/hostname ao instalador.
- `shutdown::request`, `shutdown::take_request` e `shutdown::power_off` fornecem
  um mailbox IPC de pedido de desligamento. Somente UID 0 pode solicitar; PID 1
  pode consumir pedidos e pedir ao kernel para desligar.
- A crate `imagineos_rt` contém `main!`, `print!`/`println!` e converte o
  resultado da função principal em código de saída. `main!(user_main)` chama
  `fn user_main() -> i32`, `fn user_main() -> imagineos_rt::imagineos::Result<()>`
  ou `fn user_main() -> Result<(), i32>`. Para receber argumentos sem lidar com
  ponteiros crus, use `imagineos_rt::main!(with_args user_main)` com uma função
  `fn user_main(args: imagineos_rt::Arguments<'_>,
  env: imagineos_rt::Environment<'_>) -> ...`; o iterador exclui o nome do
  executável e `env.get("NAME")` lê uma variável. Strings inválidas em UTF-8
  produzem erro. O runtime é uma crate separada da API `imagineos`, mas ainda
  não substitui a `std`; status de saída ainda é ignorado pelo kernel atual.
- A implementação de transição atual
  oferece `argument`, `environment_value`, `resolve_path`, `write`,
  `read_char`, `clear`, `read_file`, `write_file`, `list_directory`, `mkdir`,
  `touch`, `remove` e `exit`.
- As chamadas de sistema usam `int 0x80`: número em `RAX`; argumentos em
  `RDI`, `RSI`, `RDX`, `R10`, `R8` e `R9`, nessa ordem. Esse contrato é a ABI
  v2 do ImagineOS; números e semânticas não devem ser reutilizados ou alterados.
  O kernel e as aplicações compartilham as definições em
  [`shared/abi`](../shared/abi/src/lib.rs). A syscall `16` consulta a versão
  para que programas possam detectar incompatibilidade.
- A syscall `25` e `imagineos::kernel_log::read` expõem o buffer circular de
  4 KiB do kernel; o utilitário `dmesg` aceita `-n LINES` para selecionar as
  linhas mais recentes. Logs de UEFI, Limine e bootstrap ainda não são
  preservados pelo kernel.
- `shutdown` pede encerramento via `/sbin/init`; o kernel sincroniza o disco
  ATA antes do poweroff e usa o estado S5 obtido das tabelas ACPI. Ainda não há
  encerramento automático de serviços por sinais. O kernel lê registradores
  ACPI PM1 em I/O ou MMIO e o registrador Sleep Control no modo ACPI reduzido;
  a extração de `_S5_` da DSDT ainda usa um parser AML restrito.
- `signals::send`, `signals::register` e `signals::ignore` enviam ou configuram
  os sinais HUP, INT, KILL, SEGV, TERM, CONT e STOP. Handlers recebem o número
  do sinal e retornam pelo stub `sigreturn`; KILL e STOP não podem ser
  capturados/ignorados, e PID 1 é protegido. `SIGINT` é gerado por Ctrl+C na
  entrada PS/2 ou serial.
- `process::list` retorna um snapshot de até oito processos, consumido pelo
  utilitário `ps`. O kernel permite sinalizar apenas processos do mesmo UID ou
  processos de qualquer UID quando o emissor é root. Não há `top`, contabilidade
  de CPU, mascaramento de sinais, handlers aninhados ou entrega de
  exceções de CPU a handlers `SIGSEGV`. Uma falha de ring 3 encerra o processo
  afetado, sem parar o kernel.
- No login, `getty` autentica a conta e inicia o shell com um prompt
  `usuario@hostname:diretorio$` (ou `#` para UID 0). O framebuffer interpreta
  códigos ANSI SGR de primeiro plano para as cores ANSI básicas; o serial recebe
  as sequências originais. `su` inicia um shell separado após autenticar a
  conta-alvo. `sudo` executa um único comando como UID 0 e exige que a conta
  autenticada tenha a flag administrativa; a decisão é validada no kernel.
- A conta inicial é `root` com senha `root`, tanto na mídia live quanto como
  fallback. Durante `distroinstall`, é possível configurar hostname e uma
  conta adicional UID/GID 1000, além de marcar essa conta como administradora.
  A senha adicional é armazenada em `/etc/users.db` como PBKDF2-HMAC-SHA-256
  com 10.000 iterações e salt aleatório de 128 bits obtido via RDRAND. Se a CPU
  não fornecer RDRAND, a instalação configurada é recusada antes de particionar
  o disco. A senha root continua fixa em `root`; altere-a somente quando houver
  um mecanismo persistente de credenciais adequado.
- O kernel aplica bits owner/group/other de `mode` e UID/GID em leitura,
  escrita, listagem, criação, remoção, abertura e execução; UID 0 pode ignorar
  essas restrições. Administradores comuns não recebem bypass automático:
  precisam autenticar com `sudo`. A implementação atual aceita uma única conta
  adicional, um GID primário por conta e não oferece `useradd`, grupos
  suplementares, alteração de senha, `su` para sessões em outro terminal ou
  ACLs. Os processos herdam UID/GID/flag administrativa ao executar filhos.
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
  descritores de dispositivos, seek, espera/coleta de processo ou um contrato
  POSIX. Sinais não são preemptivos: são tratados ao retornar de syscalls; o
  shell também não implementa pipes nem redirecionamento.
- **Recursos de processo:** há no máximo oito slots de processo, incluindo
  processos ativos; o scheduler é cooperativo, sem preempção por timer. Um
  programa que não cede a execução pode impedir que outros avancem. `ps` é
  apenas uma fotografia dos processos ativos, sem métricas de CPU ou atualização
  contínua como `top`.
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
- **Arquivos e persistência:** em uma instalação com partição GPT/DFS válida,
  a árvore USTAR é semeada no DFS e o kernel encaminha as operações de arquivo
  para o armazenamento persistente. O journal protege metadados, mas não o
  conteúdo dos arquivos; uma queda durante uma escrita pode deixar dados
  parcialmente atualizados. O filesystem limita-se a 256 nós, caminhos de
  255 bytes e 14 extents por arquivo. O importador USTAR ainda não valida o
  checksum nem importa todos os tipos de entrada. Se o DFS não montar, o
  sistema usa o USTAR como raiz e gravações ficam no overlay volátil de até
  128 nós e 4 KiB por arquivo. O kernel aplica permissões owner/group/other
  em ambas as raízes, mas não há ACLs ou grupos suplementares.
- **Estado de validação:** a implementação é experimental. Compilar e
  empacotar um programa não comprova que ele funciona no boot real; teste a
  ISO em QEMU/OVMF e verifique o comportamento no shell.

Essas limitações descrevem o runtime atual e podem mudar conforme o ABI e o
sistema evoluírem. Confira também [Estado atual e limitações](status.md) e
[Arquitetura](architecture.md).

## Listagem de arquivos

O utilitário `ls` aceita um caminho e uma opção por argumento:

```text
ls [-a] [-l] [-s] [-t] [-o] [-p] [-h] [DIR]
```

Sem `-a`, entradas cujo nome começa por `.` são omitidas. `-l` mostra tipo,
tamanho, dono/grupo e permissões; individualmente, `-s` exibe tamanho, `-t`
tipo, `-o` UID:GID e `-p` permissões. `-h` formata o tamanho com unidades
binárias. As opções longas correspondentes são `--all`, `--long`, `--size`,
`--type`, `--owner`, `--permissions` e `--human-readable`. Opções curtas
combinadas (como `-al`) e múltiplos caminhos ainda não são aceitos. As
permissões impressas são owner/group/other aplicadas pelo kernel às operações
de arquivo, diretório e execução; UID 0 pode ignorá-las.

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
`tools/assets/zap-vga16.psf`; fontes residentes no disco só poderão ser usadas
quando houver suporte a um filesystem persistente.

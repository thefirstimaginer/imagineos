# Shell

O shell e um ELF x86_64 em ring 3. A entrada vem dos syscalls `read` via COM1 ou teclado PS/2; comandos e caminhos sao intencionalmente pequenos.

| Comando | Funcao |
| --- | --- |
| `help` | Lista comandos disponiveis |
| `cd [DIR]` | Muda o diretorio atual entre diretorios existentes no RAMFS |
| `pwd` | Mostra o diretorio atual |
| `ls [OPCOES] [DIR]` | Lista `PWD` por padrao; oculta nomes iniciados por `.` |
| `export NAME=VALUE` | Define variavel exportada para processos filhos |
| `unset NAME` | Remove variavel do ambiente do shell |
| `set` | Lista variaveis atuais |
| `read NAME` | Le uma linha e salva na variavel |
| `clear` | Limpa o framebuffer |
| `pid` | Mostra o PID do shell |
| `dmesg [-n LINES]` | Exibe as mensagens recentes do buffer do kernel |
| `uname [OPCOES]` | Exibe informações do sistema e da arquitetura |
| `shutdown [--help]` | Solicita ao init o encerramento do sistema |
| `ps [--help]` | Mostra um snapshot dos processos ativos |
| `kill [-SIGNAL] PID...` | Envia um sinal suportado a um ou mais processos |
| `su [USUARIO]` | Autentica a conta-alvo e inicia um shell com essa identidade |
| `sudo COMANDO [ARG...]` | Autentica o usuário e executa um comando como root (conta administrativa) |
| `echo [-n] texto` | Escreve texto; expande `$NAME` e `${NAME}` |
| `type NAME` | Identifica builtin ou executavel encontrado por PATH |
| `exit` | Encerra o shell atual |

Comandos externos são procurados da esquerda para a direita em `PATH`, cujo
valor inicial é `/bin`. O RAMFS fornece `ls`, `cat`, `grep`, `mkdir`, `touch`,
`rm`, `vi`, `globalconf`, `distroinstall`, `dmesg`, `uname`, `shutdown`, `ps`,
`kill`, `su` e `sudo` em
`/bin`; `grep` faz busca literal e os utilitários de leitura usam buffers
limitados.

`ps` lista PID, estado (`running` ou `stopped`) e caminho do executável no
instante da chamada. O scheduler suporta no máximo oito processos e não há
atualização interativa como em `top`. `kill` aceita os sinais HUP, INT, KILL,
SEGV, TERM, CONT e STOP por número ou nome (`kill -TERM 4` ou
`kill -s SIGSTOP 4`); `kill -l` lista a seleção disponível. Sem opção, envia
TERM. PID 1 não pode ser sinalizado. `Ctrl+C` no teclado PS/2 e o byte Ctrl+C
pela serial enviam INT ao processo que está lendo o console. O init relança o
getty quando o processo atual da sessão termina.

`shutdown` envia um pedido IPC ao processo `/sbin/init` (PID 1). O init
continua supervisionando enquanto o shell executa e encaminha o pedido ao
kernel, que sincroniza o cache do disco ATA antes de solicitar o desligamento.
O pedido só é aceito para UID 0.
O pedido ainda não encerra serviços nem envia sinais aos demais processos.
O desligamento usa as tabelas ACPI fornecidas pelo firmware para localizar os
registradores de energia e o estado S5; se essas tabelas ou os registradores
necessários não estiverem disponíveis, init informa o erro e o sistema continua
ligado.

`dmesg` mostra as mensagens mais recentes registradas pelo kernel, até 4 KiB;
`dmesg -n 10` limita a saída às últimas dez linhas. Mensagens de UEFI, Limine
e do bootstrap anteriores à entrada do kernel não fazem parte desse buffer.
`uname` aceita `-s`, `-n`, `-r`, `-v`, `-m` e `-o`; `-a`/`--all` mostra todos
os campos e as opções podem ser combinadas, por exemplo `uname -snr`.

`ls` aceita `-a`/`--all` para incluir entradas ocultas, `-l`/`--long` para
mostrar todos os metadados e as opções independentes `-s`/`--size`,
`-t`/`--type`, `-o`/`--owner`, `-p`/`--permissions` e `-h`/`--human-readable`.
Por exemplo, `ls -l -a /home` mostra também dotfiles com tipo, tamanho,
UID:GID e modo. Opções curtas combinadas e múltiplos caminhos não são aceitos.
O kernel aplica permissões owner/group/other às operações de arquivo e
diretório; UID 0 ignora as restrições, enquanto um usuário comum precisa ter
bits compatíveis. A conta administrativa comum só obtém UID 0 para o comando
executado por `sudo`.

Ao iniciar, o getty pede usuário e senha; a senha é mascarada. A conta de
fallback da mídia live é `root` com senha `root`. No `distroinstall`, escolha
opcionalmente hostname, nome/senha de uma conta UID 1000 e se ela poderá usar
`sudo`. O prompt mostra `usuario@hostname:diretorio$` para conta comum e `#`
para UID 0. O framebuffer interpreta cores ANSI SGR de primeiro plano.
Senhas da conta adicional ficam em `/etc/users.db` como PBKDF2-HMAC-SHA-256
com salt via RDRAND; hardware sem RDRAND impede a instalação com nova conta.
Por segurança, não use a senha padrão `root` em uma instalação exposta.

`ls /dev` lista os dispositivos de bloco detectados. No momento, o único
dispositivo suportado é `/dev/hda` (ATA primary-master). `distroinstall`
instala o ImagineOS nesse disco após duas confirmações digitadas; a operação
apaga a tabela de partições e todos os dados existentes. Teste com a imagem
virtual criada por `make run-installer`, nunca com um disco que contenha dados
a preservar.

`globalconf` mostra e altera configurações globais:

```text
globalconf show
globalconf get keyboard
globalconf set keyboard abnt2
globalconf set charset utf-8
```

O arquivo é `/home/.global/global.conf`. `charset=utf-8` é o único charset
suportado atualmente; `keyboard` aceita `us` ou `abnt2`. Com DFS montado, a
alteração é persistida no disco; no fallback RAMFS ela dura somente até o
reboot. Para mudar o padrão incluído em novas imagens, edite
`ramfs/home/.global/global.conf`.

`vi FILE` tem modos normal, inserção e comando. Use `i` para inserir, `Esc`
para voltar ao modo normal, `h/j/k/l` ou as setas para mover, `x` para apagar,
`0`/`$` para ir ao início/fim da linha, e `:w`, `:q`, `:q!` ou `:wq` para
salvar e sair. Os arquivos editados são limitados a 4 KiB; as alterações são
persistentes somente quando o DFS está montado.

`ls` sem argumento recebe o `PWD` atual; em um build normal as mensagens de checkpoint do loader ficam ocultas. O shell ainda mostra erros fatais do kernel.

`/sbin/init` e `/sbin/getty` tambem são programas ELF em ring 3; o kernel inicia `init` como PID 1. Processos filhos recebem `argc`, `argv` e ambiente exportado pela ABI do kernel. Tokens aceitam aspas simples/duplas e escape com barra invertida; pipelines, redirecionamentos, aliases, funções e estruturas `if/for/while` ainda não existem.

O USTAR base é usado como initramfs e fallback. Com uma partição DFS válida,
`mkdir`, `touch`, `rm`, `vi` e `globalconf` usam a raiz persistente. Sem DFS,
as alterações ficam em um overlay volátil limitado a 128 nós e arquivos de
4 KiB. `cp` e `mv` ainda não estão disponíveis.

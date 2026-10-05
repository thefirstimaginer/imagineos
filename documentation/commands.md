# Shell

O shell e um ELF x86_64 em ring 3. A entrada vem dos syscalls `read` via COM1 ou teclado PS/2; comandos e caminhos sao intencionalmente pequenos.

| Comando | Funcao |
| --- | --- |
| `help` | Lista comandos disponiveis |
| `cd [DIR]` | Muda o diretorio atual entre diretorios existentes no RAMFS |
| `pwd` | Mostra o diretorio atual |
| `ls [DIR]` | Lista `PWD` por padrao; `.` e relativo ao diretorio atual |
| `export NAME=VALUE` | Define variavel exportada para processos filhos |
| `unset NAME` | Remove variavel do ambiente do shell |
| `set` | Lista variaveis atuais |
| `read NAME` | Le uma linha e salva na variavel |
| `clear` | Limpa o framebuffer |
| `pid` | Mostra o PID do shell |
| `echo [-n] texto` | Escreve texto; expande `$NAME` e `${NAME}` |
| `type NAME` | Identifica builtin ou executavel encontrado por PATH |
| `exit` | Encerra o shell e o kernel quando nao restam tarefas |

Comandos externos sao procurados da esquerda para a direita em `PATH`, cujo valor inicial e `/bin` (o unico diretorio com executaveis no RAMFS atual). O RAMFS fornece `ls`, `cat` e `grep` em `/bin`; `grep` faz busca literal e os utilitarios de leitura usam buffers limitados.

`ls` sem argumento recebe o `PWD` atual; em um build normal as mensagens de checkpoint do loader ficam ocultas. O shell ainda mostra erros fatais do kernel.

`init` e `getty` tambem sao ELFs em ring 3. Processos filhos recebem `argc`, `argv` e ambiente exportado pela ABI do kernel. Tokens aceitam aspas simples/duplas e escape com barra invertida; pipelines, redirecionamentos, aliases, funcoes e estruturas `if/for/while` ainda nao existem.

O RAMFS USTAR e somente leitura. `cp`, `mv` e `rm` so devem ser adicionados quando houver uma camada gravavel, nao como falsos built-ins.

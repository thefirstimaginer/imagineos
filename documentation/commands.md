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

Comandos externos são procurados da esquerda para a direita em `PATH`, cujo valor inicial é `/bin`. O RAMFS fornece `ls`, `cat`, `grep`, `mkdir`, `touch`, `rm`, `vi` e `globalconf` em `/bin`; `grep` faz busca literal e os utilitários de leitura usam buffers limitados.

`globalconf` mostra e altera configurações globais:

```text
globalconf show
globalconf get keyboard
globalconf set keyboard abnt2
globalconf set charset utf-8
```

O arquivo é `/home/.global/global.conf`. `charset=utf-8` é o único charset
suportado atualmente; `keyboard` aceita `us` ou `abnt2`. Alterações feitas
durante a execução ficam no overlay volátil do RAMFS. Para manter uma escolha
entre builds, edite `ramfs/home/.global/global.conf` antes de gerar a imagem.

`vi FILE` tem modos normal, inserção e comando. Use `i` para inserir, `Esc`
para voltar ao modo normal, `h/j/k/l` ou as setas para mover, `x` para apagar,
`0`/`$` para ir ao início/fim da linha, e `:w`, `:q`, `:q!` ou `:wq` para
salvar e sair. Os arquivos editados são limitados a 4 KiB e as gravações
runtime somem ao reiniciar.

`ls` sem argumento recebe o `PWD` atual; em um build normal as mensagens de checkpoint do loader ficam ocultas. O shell ainda mostra erros fatais do kernel.

`/sbin/init` e `/sbin/getty` tambem são programas ELF em ring 3; o kernel inicia `init` como PID 1. Processos filhos recebem `argc`, `argv` e ambiente exportado pela ABI do kernel. Tokens aceitam aspas simples/duplas e escape com barra invertida; pipelines, redirecionamentos, aliases, funções e estruturas `if/for/while` ainda não existem.

O USTAR base do RAMFS é somente leitura; `mkdir`, `touch` e `rm` alteram apenas um overlay volátil em memória. `cp` e `mv` dependem de uma camada de escrita persistente.

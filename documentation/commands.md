# Shell

O shell atual e um loop integrado ao kernel, executado em ring 0. A entrada vem do COM1 ou do teclado PS/2; comandos e caminhos sao intencionalmente pequenos.

| Comando | Funcao |
| --- | --- |
| `help` | Lista comandos disponiveis |
| `clear` | Limpa o framebuffer |
| `ls` | Lista arquivos basicos do initrd |
| `cat /init` | Mostra o script de init |
| `mem` | Informa que o allocator de frames esta ativo |
| `ps` | Identifica a tarefa foreground atual |
| `echo texto` | Escreve texto no console |

Os scripts `/init` e `/getty` aceitam `echo` e `exec /arquivo`. Isso ainda nao e execucao de processos ELF.

# Shell

O shell e um ELF x86_64 em ring 3. A entrada vem dos syscalls `read` via COM1 ou teclado PS/2; comandos e caminhos sao intencionalmente pequenos.

| Comando | Funcao |
| --- | --- |
| `help` | Lista comandos disponiveis |
| `clear` | Limpa o framebuffer |
| `pid` | Mostra o PID do shell |
| `echo texto` | Escreve texto no console |
| `exit` | Encerra o shell e o kernel quando nao restam tarefas |

`init` e `getty` tambem sao ELFs em ring 3; os tres usam a ABI `int 0x80`.

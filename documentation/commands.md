# Comandos do shell

O shell atual e uma aplicacao em `userspace/shell/main.c`.

| Comando | Funcao | Estado |
| --- | --- | --- |
| `help` | Lista os comandos disponiveis | Funcional em execucoes estaveis |
| `test` | Verifica que o shell esta em userspace | Funcional em execucoes estaveis |
| `echo TEXT` | Imprime texto | Funcional em execucoes estaveis |
| `clear` | Limpa o terminal por syscall | Funcional em execucoes estaveis |
| `exit` | Encerra o shell | Depende do lifecycle de processos |
| `proc-test` | Exercita `fork`, `waitpid` e `exit` | Instavel; pode reiniciar a VM |

O parser atual e intencionalmente simples. Ainda nao ha argumentos gerais, redirecionamento, pipes, historico ou filesystem.

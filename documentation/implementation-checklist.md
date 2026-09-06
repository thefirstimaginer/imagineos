# Checklist de implementacao do userspace

Esta lista transforma a pesquisa sobre init, servicos, TTY e shell em tarefas verificaveis para o estado atual do ImagineOS.

## Estado atual

| Item | Estado | Evidencia ou bloqueio |
| --- | --- | --- |
| `init` PID 1 e configuracao `initfile.ini` | Implementado | Configuracao embutida no ELF; `$shell` inicia `shell.service` |
| Shell em userspace | Implementado | `userspace/shell/main.c` |
| TTY com leitura por linha | Parcial | TTY faz buffer, echo e backspace em userspace |
| TTY canonico no kernel | Parcial | `input_read` espera newline, mas `read_nonblock` expoe a fila bruta |
| Echo | Implementado | Shell escreve os caracteres recebidos |
| Cursor no campo ativo | Implementado | Shell e login exibem cursor piscante durante leitura |
| Backspace | Implementado | Tratado por `userspace/shell/tty.c` |
| Limite de linha | Implementado | TTY descarta o excedente ate `Enter` |
| `fork` | Experimental | Filho agora possui caminho de restauracao de frame completo |
| `waitpid` | Experimental | Servicos recolhem filhos; teste interativo ainda necessario |
| `exit` | Experimental | Pai agora e retomado por `user_resume`; teste interativo ainda necessario |
| `execve` | Ausente | Existe apenas `exec_service` para modulos conhecidos |
| `getty` | Parcial | Servico de console inicia `login.service`; nao abre `/dev/tty` |
| `login` | Parcial | Aceita qualquer usuario em modo desenvolvimento; nao usa `/etc/passwd` |
| `/dev/tty`, `/dev/null`, `/dev/zero` | Ausente | Falta VFS/device filesystem |
| `mount`, `/proc`, `/sys` | Ausente | Falta filesystem e syscalls de montagem |
| Sinais (`Ctrl+C`, `SIGINT`) | Ausente | Falta modelo de sinais, grupos e entrega ao processo |
| Display server em userspace | Bloqueado | Depende de processos, memoria compartilhada, input e framebuffer estaveis |

## Ordem de implementacao

### 1. TTY e shell

- [x] Manter o TTY como biblioteca de userspace enquanto nao existe device API.
- [x] Bufferizar uma linha ate `Enter`.
- [x] Fazer echo, backspace e limite de capacidade.
- [ ] Mover a disciplina canonica para o kernel.
- [ ] Adicionar fila de saida, controle de cursor e redimensionamento.
- [ ] Definir comportamento para `Ctrl+C` depois de implementar sinais.

### 2. Init e servicos

- [x] Embutir `userspace/init/initfile/initfile.ini` no ELF do init.
- [x] Interpretar `SERVICE ($getty) START`.
- [x] Carregar getty, login e shell como servicos conhecidos.
- [ ] Suportar varias diretivas e ordem de inicializacao.
- [ ] Definir `DAEMON`, restart policy e status do servico.
- [ ] Impedir que um servico instavel derrube o PID 1.

### 3. Getty e login

- [x] Implementar `getty` como modulo ELF de console.
- [x] Implementar `login` sem senha para testes iniciais.
- [ ] Implementar uma API de TTY no kernel.
- [ ] Implementar `execve` de ELF por caminho.
- [ ] Adicionar `dup2`/redirecionamento de stdin, stdout e stderr.
- [ ] Adicionar filesystem ou imagem de arquivos para `/etc/passwd`.

### 4. Kernel necessario

- [x] Adicionar handlers diagnosticos para excecoes de CPU 0-31.
- [x] Exibir vetor e `CR2` em page faults antes de parar a CPU.
- [x] Desativar `ENDBR64`/CET e red zone na compilacao freestanding.
- [x] Exibir RIP, CS, RFLAGS, RSP, SS, erro e CR3 no handler de excecao.
- [ ] Corrigir frame de interrupcao timer vindo de ring 3.
- [ ] Estabilizar `CR3`, stack e frame de `fork`.
- [x] Restaurar todos os registradores de um `UserFrame` ao retomar userspace.
- [x] Carregar servicos no slot fisico do filho sem sobrescrever a imagem do pai.
- [ ] Fazer `waitpid` bloquear e acordar por evento.
- [ ] Fazer `exit` retornar ao pai sem `user_enter` ad hoc.
- [x] Manter init, getty e login supervisionando e recolhendo seus servicos.
- [ ] Implementar handlers de page fault e general protection fault.
- [ ] Adicionar testes repetidos de boot e processos.

### 5. Dispositivos e grafico

- [ ] Expor teclado/input por filas de eventos.
- [ ] Expor framebuffer por mapeamento controlado.
- [ ] Implementar memoria compartilhada ou buffers IPC.
- [ ] Criar display server minimo em userspace.
- [ ] Criar clientes graficos somente depois dos testes de processo passarem.

## Criterio de conclusao

A base de userspace sera considerada pronta para `getty`, `login` e display server quando:

- 100 boots consecutivos nao produzirem `invalid init.elf` nem reset;
- `fork`, `waitpid` e `exit` passarem repetidamente;
- dois processos puderem coexistir sem corromper RIP, RSP ou CR3;
- page faults gerarem diagnostico legivel, nao triple fault;
- um ELF puder ser carregado por caminho ou filesystem;
- TTY e stdin/stdout tiverem contrato definido no kernel.

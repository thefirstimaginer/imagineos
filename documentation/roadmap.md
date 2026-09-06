# Roadmap

A prioridade e estabilidade do userspace. O desenvolvimento grafico deve aguardar a conclusao das etapas de processo, memoria e interrupcoes.

## Fase 1: diagnostico confiavel

- adicionar handlers para excecoes 0 a 31;
- registrar vetor, erro, RIP, RSP, CR2 e CR3;
- criar um panic path que pare a CPU sem mascarar a causa;
- separar logs VGA e serial;
- criar um teste de boot repetido no host.

## Fase 2: entrada e retorno de interrupcao

- substituir o wrapper atual por um frame de interrupcao definido em Assembly;
- preservar todos os registradores, incluindo os usados pelo ABI;
- distinguir frame de kernel e frame vindo de ring 3;
- restaurar somente o frame selecionado;
- testar timer sem troca de processo e depois com troca controlada.

## Fase 3: processo minimo confiavel

- definir claramente estados `READY`, `RUNNING`, `BLOCKED` e `TERMINATED`;
- implementar `fork` com frame filho completo;
- validar que pai e filho possuem CR3, stack e imagem corretos;
- fazer `waitpid` dormir e acordar por evento, sem busy loop;
- fazer `exit` retornar ao pai sem chamar `user_enter` de forma ad hoc;
- liberar PCB, stack e address space somente depois do reap.

## Fase 4: memoria virtual

- criar um gerenciador de frames fisicos;
- mapear segmentos ELF individualmente com permissoes corretas;
- separar codigo, dados, BSS, stack e heap;
- implementar page fault de usuario;
- adicionar copy-on-write para `fork`;
- remover dependencias de enderecos fisicos fixos.

## Fase 5: runtime de userspace

- ampliar libc e tratamento de errno;
- criar testes executaveis de syscall;
- adicionar execucao de utilities como processos independentes;
- implementar pipes ou IPC basico;
- criar um servico de logging e diagnostico;
- estabilizar shell e gerenciamento de comandos.

## Fase 6: dispositivos para userspace

- definir uma API de input segura;
- expor framebuffer por syscall ou device file;
- implementar mapeamento controlado de memoria de video;
- adicionar filas de eventos de teclado e mouse;
- testar um display server minimo sem janelas.

## Fase 7: display server

Somente depois das fases anteriores:

- criar `displayd` em userspace;
- definir protocolo de clientes;
- implementar buffers por processo;
- adicionar composicao de superficies;
- criar uma primeira aplicacao grafica de teste.

## Criterio para iniciar grafico

O trabalho grafico em userspace deve comecar somente quando todos estes testes passarem repetidamente:

- pelo menos 100 boots sem `invalid init.elf` ou reset;
- `init` e shell iniciam sem depender de timing;
- `fork`/`waitpid`/`exit` passam varias vezes;
- dois processos de usuario coexistem sem corromper CR3;
- page fault de usuario gera diagnostico, nao triple fault;
- utilities podem ser iniciadas e encerradas repetidamente.

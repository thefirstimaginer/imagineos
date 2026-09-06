# Estado atual e limitacoes

## O que foi documentado

Esta documentacao registra:

- fluxo de boot e carregamento de modulos;
- organizacao entre kernel, drivers, libc e userspace;
- modelo atual de paging e CR3;
- entrada de syscalls e interrupcoes;
- comandos de build, execucao e diagnostico;
- capacidades observadas no QEMU;
- problemas conhecidos e prioridades futuras.

A documentacao antiga foi removida porque continha caminhos de uma arquitetura anterior e planos que nao correspondiam mais a arvore atual.

## O que funciona

- Compilacao limpa da ISO com `make clean && make iso`.
- Boot do kernel em QEMU em varias execucoes.
- Inicializacao de GDT/TSS, IDT, PIC, timer e syscall MSRs.
- Leitura dos modulos ELF Multiboot e validacao basica de seus headers.
- Entrada de programas em ring 3 em execucoes bem-sucedidas.
- Shell textual e leitura de teclado PS/2 em execucoes bem-sucedidas.
- `help`, `test`, `echo`, `clear` e `exit` em execucoes bem-sucedidas.
- A syscall de limpeza do terminal funciona diretamente pelo shell.

## Limitacoes atuais

### Loader e init

O sistema ainda pode imprimir `invalid init.elf`, travar durante a inicializacao ou reiniciar a VM. O problema nao e tempo de procura: o modulo ja esta presente na memoria quando o kernel inicia. As areas de risco sao o parser Multiboot/ELF, a copia para o staging em `0x400000`, a criacao das page tables e a entrada em ring 3. A compilacao agora desativa `ENDBR64`/CET e red zone, que eram incompatíveis com o ambiente freestanding observado durante o boot.

### Processos

`fork`, `waitpid` e `exit` ainda nao formam um ciclo de vida comprovadamente confiavel. O retorno entre processos agora usa `user_resume`, que restaura o `UserFrame` completo. O loader de servicos tambem foi corrigido para escrever no slot fisico do filho, sem sobrescrever a imagem virtual do pai. O teste interativo de `exit` e `proc-test` ainda precisa confirmar o comportamento no QEMU.

O login vazio agora permanece no prompt em vez de chamar `exit`, evitando usar prematuramente o retorno de processo durante a cadeia `getty -> login -> shell`.

### Scheduler e frames

A troca de contexto preemptiva usa um frame de timer compartilhado com o frame salvo pela CPU. A restauracao pode corromper RIP, RSP ou CR3. O timer continua sendo necessario para o sistema, mas o escalonamento de userspace deve permanecer desativado ou limitado ate haver um frame de contexto dedicado e validado.

### Memoria

- page tables sao estaticas;
- ha somente 16 espacos de usuario;
- a imagem usa uma janela fixa de 2 MiB;
- nao ha heap de usuario robusto;
- nao ha copy-on-write;
- nao ha mapeamento fino de segmentos ELF;
- nao ha handlers completos de page fault e general protection fault.

### Sistema de arquivos e dispositivos

Nao existe filesystem de usuario, VFS, armazenamento persistente, mouse, framebuffer seguro ou API grafica de userspace. Os programas sao carregados como modulos definidos na ISO.

## Estado de estabilidade

O projeto esta adequado para investigar boot, syscalls simples, terminal e loader ELF, mas nao esta pronto para:

- executar processos filhos de forma confiavel;
- executar servicos repetidamente;
- oferecer um display server em userspace;
- iniciar desenvolvimento de um driver grafico semelhante ao Xorg.

Qualquer resultado positivo no shell deve ser tratado como um teste isolado, nao como prova de estabilidade geral.

## Problemas conhecidos

1. Reinicio intermitente antes ou durante o carregamento do `init.elf`.
2. Page fault/triple fault durante `proc-test` ou ao encerrar um servico.
3. Possivel corrupcao no retorno de interrupcoes de ring 3; o handler agora mostra RIP, CS, RFLAGS, RSP, SS, erro e CR3.
4. Trocas de CR3 e frames de processo ainda insuficientemente testadas.
5. Ausencia de relatorio de panic legivel para excecoes de CPU.
6. Falta de testes automatizados de boot e de syscalls em guest.
7. Uso de memoria fixa para processos, stacks e page tables.

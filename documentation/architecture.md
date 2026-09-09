# Arquitetura

## Boot

O GRUB carrega o kernel e os programas de usuario como modulos Multiboot 2:

- `imos.elf`: kernel;
- `init.elf`: primeiro programa de usuario;
- `shell.elf`: shell;
- `clear.elf`: utility de limpeza do terminal.

O fluxo de boot e:

1. O Assembly verifica Multiboot, CPUID e long mode.
2. Paging inicial identity-mapped e ativado.
3. O kernel configura syscall MSRs, TSS, video, processos, input, IDT e scheduler.
4. `kernel/src/user.c` procura o modulo `init.elf`.
5. O segmento ELF e copiado para a imagem de usuario em `0x400000`.
6. Um espaco de endereco e criado e o processo init entra em ring 3.

## Kernel

As responsabilidades principais estao organizadas assim:

- `kernel/main.c`: inicializacao geral e entrada do kernel.
- `kernel/src/user.c`: parser Multiboot, loader ELF e execucao de servicos.
- `kernel/src/paging.c`: page tables, CR3 e imagem fisica dos processos.
- `kernel/src/process.c`: PCB, estados, PID e ciclo de vida.
- `kernel/src/scheduler.c`: ticks PIT e tentativa de escalonamento.
- `kernel/src/syscall_dispatch.c`: dispatch de syscalls.
- `drivers/src/print.c`: terminal VGA textual.
- `kernel/src/input.c` e `drivers/src/ps2.c`: entrada PS/2.

## Userspace

A imagem de usuario e ligada em `0x400000`. A libc minima oferece wrappers para:

- `read`, `read_nonblock` e `write`;
- `exec_service`;
- `fork`, `waitpid` e `exit`;
- `get_ticks`.

O `init` le a configuracao `userspace/init/initfile/initfile.ini`, embutida no
ELF durante o link, e procura uma diretiva `SERVICE ($nome) START`. O nome
`$getty` e convertido para `getty.service`. O getty inicia `login.service`, que
aceita um nome de usuario em modo de desenvolvimento e inicia `shell.service`.
Esses servicos usam o console padrao; ainda nao existe `/dev/tty` nem um
filesystem de usuarios. A utility `clear` possui seu proprio ELF e implementa
diretamente a syscall de limpeza.

## Memoria

Cada espaco de usuario possui tabelas estaticas com ate 16 slots. A imagem de usuario ocupa uma janela fisica de 2 MiB por slot, copiada a partir do staging virtual em `0x400000`.

O espaco do kernel e identity-mapped. A imagem de usuario e mapeada como pagina grande com permissao de usuario. Este modelo e deliberadamente simples e ainda nao oferece protecao fina por segmento, heap dinamico ou alocacao de paginas sob demanda.

## Syscalls e interrupcoes

A entrada de syscall fica em `arch/x86_64/src/syscall_entry.asm`. O timer usa um frame de interrupcao definido em `kernel/include/scheduler.h` e wrappers em `arch/x86_64/src/idt_.asm`.

A preservacao e restauracao desse frame ainda e uma area instavel. Um erro nessa fronteira pode corromper RIP, CR3 ou a pilha e produzir page fault, general protection fault ou triple fault.

Os vetores de excecao 0 a 31 agora possuem handlers diagnosticos. Em especial,
page fault imprime o vetor e o endereco em `CR2` e para a CPU. Isso evita que a
causa seja mascarada imediatamente por um triple fault, embora ainda nao haja
recuperacao do processo que falhou.

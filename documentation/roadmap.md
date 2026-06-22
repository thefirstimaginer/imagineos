# ImagineOS Roadmap

Este roadmap está baseado no estado atual do código e nas prioridades reais da base de código. Ele agrupa as implementações por prioridade, indica onde trabalhar em cada função e sugere materiais para aprendizado.

---

## 1. Prioridade Alta: Estabilidade do kernel

### 1.1 Processo e agendamento
- Objetivo: retirar alocação fixa de `process.c` e permitir múltiplos processos.
- Por que fazer: o kernel atualmente cria todos os processos em `0x100000` e usa um scheduler round-robin muito simples.
- Onde implementar:
  - `main/management/process.c`
  - `include/management/process.h`
  - `main/management/scheduler.c`
  - `include/management/scheduler.h`
- Funções recomendadas:
  - `process_create()` → usar heap/heap manager em vez de endereço fixo.
  - `process_switch()` → garantir salvamento/restauração de contexto sem corrupção.
  - `scheduler_schedule()` → adicionar verificação de estados e, no futuro, prioridade.
- Resultado esperado:
  - suportar vários processos simultâneos
  - permitir idle process e processo usuário separados
  - preparar para `waitpid`, `exit`, e políticas de prioridade

### 1.2 Interrupções e exceções
- Objetivo: suportar mais handlers de exceção e melhorar o IDT.
- Por que fazer: hoje o IDT só registra timer e teclado.
- Onde implementar:
  - `main/devices/idt.c`
  - `main/boot/quin-headers/main.asm`
- Funções recomendadas:
  - adicionar `idt_set_entry()` para exceções 0–31
  - criar handlers para `#GP`, `#PF`, `#DE`, `#DF`
  - manter `pic_eoi_master()`/`pic_eoi_slave()` consistentemente
- Resultado esperado:
  - kernel capaz de depurar falhas de page fault e proteção geral
  - base para panic handler e mensagens de erro

### 1.3 Gerenciamento de memória básica
- Objetivo: começar uma infraestrutura de alocação dinâmica.
- Por que fazer: sem `malloc`/`free` viáveis, `process.c` depende de memória estática e fixa.
- Onde implementar:
  - `include/libraries/libimagine.h`
  - `include/libraries/math.h` (incompleto, mas o `libimagine.h` já declara `malloc`/`free`)
  - novo arquivo sugerido: `main/management/memory.c`
  - `main/boot/quin-headers/main.asm` já inicializa paging em nível mínimo
- Funções recomendadas:
  - `malloc()`, `realloc()`, `free()`
  - `kalloc()`, `kfree()` (separado para o kernel)
- Resultado esperado:
  - habilitar alocação de stacks de processo
  - permitir listas dinâmicas e buffers de entrada

---

## 2. Prioridade Média: Infraestrutura do sistema

### 2.1 Shell e usuário
- Objetivo: completar QBshell e tornar comandos utilizáveis.
- Por que fazer: a shell existe, mas está incompleta e sem parsing robusto.
- Onde implementar:
  - `userspace/QBshell/shell.c`
  - `userspace/QBshell/tty.c`
  - possivelmente `include/userspace/QBshell/tty.h`
- Funções recomendadas:
  - `shell_init()`
  - `qbs_commands()` → adicionar parsing, redirecionamento básico e histórico
  - `qbs_echo_prompt()` → melhorar prompt e feedback
- Resultado esperado:
  - comandos `pwd`, `list`, `cd`, `exec`, `exit`, `kill`, `echo`
  - suporte básico a argumentos e retorno de erro

### 2.2 Vídeo e modo gráfico
- Objetivo: restaurar o suporte gráfico básico e limpar referências orfãs.
- Por que fazer: `graphics.h` existe, mas foi comentado no kernel e nenhum `graphics.c` é visível.
- Onde implementar:
  - `include/devices/graphics.h`
  - criar `main/devices/graphics.c`
  - atualizar `main/kernel/main.c` e `main/devices/print.c`
- Funções recomendadas:
  - `graphics_init()`
  - `graphics_put_pixel()`
  - `graphics_draw_char()` / `graphics_draw_string()`
- Resultado esperado:
  - driver VGA simples em modo gráfico ou modo texto melhorado
  - remover comentários de importação em `main/kernel/main.c`

### 2.3 Drivers e dispositivos básicos
- Objetivo: estabilizar entrada, PIC, RTC e portas.
- Por que fazer: eles são a base do kernel e já existem implementações principais.
- Onde implementar:
  - `main/devices/keyboard.c`
  - `main/devices/ps2.c`
  - `main/devices/pic.c`
  - `main/devices/port.c`
  - `main/devices/rtc.c`
- Funções recomendadas:
  - `keyboard_init()` / `keyboard_set_handler()`
  - `pic_remap()` / `pic_eoi_master()`
  - `port_inb()` / `port_outb()`
- Resultado esperado:
  - teclado funcional e sem overflow de buffer
  - timer interrupt confiável para scheduler

---

## 3. Prioridade Baixa / Futura: Recursos avançados

### 3.1 Filesystem e armazenamento
- Objetivo: planejar VFS e FAT32 como próximo grande passo.
- Por que fazer: atualmente não há abstração de arquivos nem persistência.
- Onde implementar:
  - novo diretório sugerido: `include/fs/`, `main/fs/`
  - usar módulos existentes como exemplo de estrutura
- Funções recomendadas:
  - `open()`, `read()`, `write()`, `close()`
  - VFS layer com operações genéricas de arquivo
- Resultado esperado:
  - protótipo de RAM disk ou FAT32 read-only

### 3.2 Biblioteca padrão
- Objetivo: completar `math.h`, `stdio.h`, `string.h` e `stdlib`.
- Por que fazer: há cabeçalhos declarados, mas implementações ausentes ou parciais.
- Onde implementar:
  - `include/libraries/math.h`
  - `include/libraries/stdio.h`
  - `include/libraries/string.h`
  - `include/libraries/libimagine.h`
- Funções recomendadas:
  - `strlen()`, `strcmp()`, `memcpy()`, `memset()`
  - `printf()` e `malloc()` no kernel/userland
- Resultado esperado:
  - suporte básico de string e I/O para aplicações de usuário

### 3.3 Modo gráfico avançado e UEFI (futuro)
- Objetivo: planejar VESA/UEFI após a base de modo texto e básico estar estável.
- Onde implementar:
  - `main/boot/quin-headers/main.asm` e módulos de boot
  - novo driver gráfico em `main/devices/`
- Resultado esperado:
  - mode set VESA suportado ou fallback gráfico seguro

---

## 4. Estrutura recomendada para implementação

1. Kernel e scheduler
   - `main/management/process.c`
   - `main/management/scheduler.c`
   - `main/devices/idt.c`
2. Memória e alocação
   - `include/libraries/libimagine.h`
   - novo `main/management/memory.c`
3. Shell e aplicativos
   - `userspace/QBshell/shell.c`
   - `userspace/QBshell/tty.c`
4. Vídeo e dispositivos
   - `include/devices/graphics.h`
   - `main/devices/print.c`
   - `main/devices/keyboard.c`
5. Driver/infra adicional
   - `main/devices/pic.c`
   - `main/devices/port.c`
   - `main/devices/rtc.c`

---

## 5. Materiais recomendados

- OSDev Wiki: https://wiki.osdev.org/ (principal referência para boot, GDT, IDT, PIC, paging)
- "Operating Systems: Three Easy Pieces" (para conceitos de scheduler e processos)
- xv6 MIT: https://github.com/mit-pdos/xv6-public (bom para processos, syscall e filesystem)
- Intel 64 and IA-32 Architectures Software Developer's Manual (para long mode, paging, CR3, IDT)
- "The little book about OS development" / "Bare Bones" tutorials
- VESA/VBE programming guides (para modo gráfico) e QEMU com `-vga std` para testes
- C standard library mini-implementações: `musl`, `newlib` ou `klibc`

---

## 6. Recomendações de prioridades de implementação

### Alta prioridade
- remover alocação fixa de processos
- adicionar exception handlers no IDT
- tornar o timer interrupt útil para scheduling
- estabilizar driver de teclado e PIC

### Média prioridade
- criar alocador básico (`malloc`/`free`)
- melhorar QBshell e parsing de comandos
- restaurar driver gráfico simples
- limpar imports e módulos órfãos

### Baixa prioridade / longo prazo
- implementar filesystem e VFS
- adicionar suporte completo a `stdin`/`stdout` em apps
- desenvolver shell scripting e histórico avançado
- estender para UEFI e VESA

---

## 7. Observações específicas do repositório atual

- O arquivo `main/boot/quin-headers/main.asm` já contém código de paginação e long mode; use isso como referência para a transição para um kernel 64-bit.
- `include/devices/graphics.h` define a interface de vídeo, mas o driver concreto não está presente.
- `include/libraries/libimagine.h` já define `malloc`, `realloc` e `free`, portanto o kernel espera que essas funções sejam implementadas em breve.
- `main/management/modules.c` tem código de carregamento de módulos, mas `modules_output()` não libera recursos nem exibe lista completa.
- `userspace/QBshell/shell.c` já tem a estrutura de comando, mas muitas rotas de comando ainda estão incompletas.

---

### Como usar este roadmap
1. Escolha um item de alta prioridade e implemente no código atual.
2. Teste no QEMU com a saída do kernel e verifique interrupções/exceptions.
3. Avance para as melhorias de infraestrutura de memória e shell.
4. Use os materiais recomendados para aprofundar conceitos específicos antes de escrever o código.

# ImagineOS Roadmap Complementar

Este arquivo é um roadmap alternativo e não altera o roadmap existente. Ele está organizado por prioridade de função, por onde trabalhar no código atual, e inclui recomendações de estudo específicas.

---

## 1. Prioridade Crítica

### 1.1 Orquestração de processos e agendamento
- Por que: o kernel depende de uma alocação de processo fixa (`0x100000`) e não pode criar múltiplos processos confiáveis.
- Trabalhar em:
  - `main/management/process.c`
  - `include/management/process.h`
  - `main/management/scheduler.c`
- O que implementar:
  - alocação dinâmica para estruturas `Process`
  - identificação de PID único e lista de processos segura
  - scheduler round-robin com verificação de estado `PROCESS_READY`
  - `process_yield()` com proteção contra processo único
- Aprender:
  - conceitos de Process Control Block (PCB)
  - escalonamento round-robin e estados de processo

### 1.2 Interrupções e tratamento de exceções
- Por que: o IDT atual registra apenas timer e teclado.
- Trabalhar em:
  - `main/devices/idt.c`
  - `main/boot/quin-headers/main.asm`
- O que implementar:
  - definição completa de entradas IDT para vetores 0–31
  - handlers básicos para `#PF`, `#GP`, `#DE`, `#DF`
  - uso correto de `pic_eoi_master()` após cada interrupção
- Aprender:
  - estrutura do IDT e como montar descriptors de interrupção
  - diferença entre interrupções de hardware e exceções da CPU

### 1.3 Alocação de memória básica do kernel
- Por que: o sistema declara `malloc`/`free` em `include/libraries/libimagine.h`, mas não há implementação visível.
- Trabalhar em:
  - `include/libraries/libimagine.h`
  - novo módulo sugerido: `main/management/memory.c`
- O que implementar:
  - allocator simples de heap no kernel
  - cabeçalhos de blocos e free list básica
  - integração mínima com `process_create()` para stacks de processo
- Aprender:
  - alocadores de heap simples (bitmap ou lista encadeada)
  - alinhamento de memória e headers de alocação

---

## 2. Prioridade Alta

### 2.1 Inicialização de hardware e PIC
- Por que: o timer e teclado dependem de PIC e PIT configurados corretamente.
- Trabalhar em:
  - `main/devices/pic.c`
  - `main/devices/keyboard.c`
  - `main/devices/ps2.c`
  - `main/devices/port.c`
- O que implementar:
  - mapeamento PIC sem conflitos com IRQs
  - handler de teclado com bounds-check para scancodes
  - inicialização de PIT para ticks periódicos
- Aprender:
  - PIC remapeado e manejo de EOI
  - entrada PS/2 e tradução de scancode

### 2.2 Console e saída de texto
- Por que: a shell e os módulos precisam de saída estável.
- Trabalhar em:
  - `main/devices/print.c`
  - `userspace/QBshell/tty.c`
  - `include/devices/graphics.h`
- O que implementar:
  - limpeza e estrutura do driver de texto
  - possíveis logos do modo gráfico simples
  - remover referências inválidas de `graphics.h` ou restaurar driver
- Aprender:
  - modo texto VGA e mapeamento de memória em `0xB8000`
  - desenho básico de pixels e fontes bitmap

---

## 3. Prioridade Média

### 3.1 Shell e interação do usuário
- Por que: a shell é o ponto de entrada para o usuário e precisa de comando robusto.
- Trabalhar em:
  - `userspace/QBshell/shell.c`
  - `userspace/QBshell/tty.c`
- O que implementar:
  - parsing de comandos simples
  - suporte para `help`, `date`, `history`, e mensagens de erro
  - prompt funcional e feedback de entrada
- Aprender:
  - design de shells simples
  - parsing de tokens e execução de comandos básicos

### 3.2 Módulos e carregamento de aplicações
- Por que: `main/management/modules.c` tem código incompleto e falha em listar módulos corretamente.
- Trabalhar em:
  - `main/management/modules.c`
- O que implementar:
  - inicialização completa de módulos listados
  - saída de módulos carregados
  - cleanup adequado para evitar vazamentos de recursos
- Aprender:
  - abstração de módulos e tabelas de função
  - padrão `init/run` para componentes de kernel

---

## 4. Prioridade Baixa / Longo prazo

### 4.1 Sistema de arquivos e persistência
- Por que: não há filesystem no repositório atual.
- Onde começar:
  - criar novas pastas como `include/fs/` e `main/fs/`
- O que planejar:
  - VFS básico com operações `open/read/write/close`
  - protótipo de RAM disk ou FAT32 read-only
- Aprender:
  - arquitetura VFS e camadas de abstração de arquivos
  - FAT32 boot sector, clusters e diretórios

### 4.2 Biblioteca padrão C
- Por que: `math.h` está vazio e cabeçalhos declarados dependem de funções básicas.
- Onde trabalhar:
  - `include/libraries/math.h`
  - `include/libraries/stdio.h`
  - `include/libraries/string.h`
- O que implementar:
  - funções essenciais de string e memória
  - `printf()` simples e `malloc()`/`free()` em `libimagine`
- Aprender:
  - construção de `libc` minimalista para kernels
  - implementação de `printf` e manipuladores de formato

### 4.3 Modo gráfico e recursos visuais
- Por que: há interface `graphics.h`, mas sem driver funcional.
- Onde trabalhar:
  - `include/devices/graphics.h`
  - `main/devices/graphics.c` (novo)
  - `main/kernel/main.c`
- O que implementar:
  - inicialização de modo gráfico VESA ou driver VGA simples
  - desenho de pixel e texto em framebuffer
- Aprender:
  - VESA BIOS Extensions (VBE) e modo linear framebuffer
  - drivers de vídeo em x86

---

## 5. Recomendações técnicas e materiais

- OSDev Wiki: https://wiki.osdev.org/
- xv6 MIT: https://github.com/mit-pdos/xv6-public
- "Operating Systems: Three Easy Pieces"
- Intel 64 Architecture Manual (especialmente capítulos de proteção, IDT, interrupts e paging)
- Tutoriais de VESA/VBE para modo gráfico
- Exemplos de `libc` mínima: `newlib`, `musl`, `klib`
- Documentos sobre `PIC`, `PIT`, `PS/2`

---

## 6. Sugestão de estrutura para usar este roadmap

1. Conclua primeiro a infraestrutura de processos e o IDT.
2. Implemente heap básico para suporte a alocação dinâmica.
3. Estabilize drivers de hardware essenciais (PIC, teclado, timer).
4. Avance para shell e interface de usuário.
5. Planeje filesystem e biblioteca padrão como próximos grandes marcos.

---

## 7. Arquivos críticos do repositório atual

- `main/kernel/main.c`
- `main/management/process.c`
- `main/management/scheduler.c`
- `main/devices/idt.c`
- `main/devices/keyboard.c`
- `main/devices/print.c`
- `include/libraries/libimagine.h`
- `include/devices/graphics.h`
- `userspace/QBshell/shell.c`
- `main/management/modules.c`

Este roadmap foi escrito sem modificar o arquivo `documentation/roadmap.md` já existente.

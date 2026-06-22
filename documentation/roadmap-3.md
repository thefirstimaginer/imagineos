# ImagineOS Roadmap Semanal

Este roadmap apresenta metas semanais de implementação para o projeto, baseado na prioridade das funções e no estado atual da base de código.

---

## Semana 1: Fundamentos do kernel

### Objetivo principal
Estabilizar o kernel mínimo e preparar a infraestrutura de processos.

### Metas
- [ ] Revisar e melhorar `main/management/process.c`
  - remover alocação fixa em `0x100000`
  - garantir criação de processos com PID único
  - validar `process_switch()` e `process_yield()`
- [ ] Melhorar o scheduler em `main/management/scheduler.c`
  - confirmar round-robin
  - proteger `PROCESS_READY` e `PROCESS_RUNNING`
- [ ] Inspecionar `main/devices/idt.c`
  - adicionar comentários claros sobre vetores e handlers
  - manter timer e teclado funcionando sem regressão

### Entregáveis
- Kernel inicializa sem travar após `kernel_main()`
- Dois processos podem ser criados sequencialmente
- Scheduler alterna entre processos prontos

---

## Semana 2: Interrupções e memória básica

### Objetivo principal
Adicionar suporte a exceções e começar alocação de memória do kernel.

### Metas
- [ ] Implementar handlers de exceção no IDT
  - `#GP`, `#PF`, `#DE`, `#DF`
  - mensagens de panics ou `hlt` seguro
- [ ] Revisar boot e paginação em `main/boot/quin-headers/main.asm`
  - entender o setup atual de page tables
  - documentar quais tabelas existem e por que o kernel usa long mode
- [ ] Criar esqueleto de gerenciador de memória
  - novo arquivo sugerido: `main/management/memory.c`
  - definir `kalloc()` e `kfree()`

### Entregáveis
- Kernel lida com exceções sem travar de forma silenciosa
- Documentação curta de como o boot inicializa paging
- Alocador básico disponível para o kernel

---

## Semana 3: Shell, módulos e I/O básico

### Objetivo principal
Melhorar a interface do usuário e o carregamento de módulos.

### Metas
- [ ] Completar `userspace/QBshell/shell.c`
  - parsing mínimo de comandos
  - `help`, `date`, `history` funcionando corretamente
- [ ] Revisar `main/management/modules.c`
  - corrigir `modules_output()` para exibir os módulos
  - adicionar cleanup ou status de carregamento
- [ ] Validar `main/devices/keyboard.c`
  - adicionar bounds checking em scancodes
  - confirmar que `keyboard_set_handler()` chama o handler corretamente

### Entregáveis
- Shell responde a comandos básicos sem falhas
- Lista de módulos funcional na inicialização
- Entrada de teclado estável

---

## Semana 4: Vídeo, driver gráfico e biblioteca C

### Objetivo principal
Restaurar ou estabilizar a saída de vídeo e componentes básicos da biblioteca padrão.

### Metas
- [ ] Restaurar driver gráfico básico
  - criar `main/devices/graphics.c` se necessário
  - implementar funções fundamentais definidas em `include/devices/graphics.h`
- [ ] Melhorar `main/devices/print.c`
  - garantir que modo texto e/ou gráfico possam coexistir
  - remover includes comentados se não forem usados
- [ ] Implementar funções básicas de `libimagine`
  - `strlen()`, `strcmp()`, `memcpy()`, `memset()`
  - se possível, iniciar `malloc()`/`free()`

### Entregáveis
- Saída de texto limpa e previsível no console
- Driver gráfico básico definido e compilável
- Biblioteca de runtime do kernel com string helpers

---

## Semana 5: Estrutura de arquivos e progresso de sistemas

### Objetivo principal
Planejar filesystem e consolidar subsistemas essenciais.

### Metas
- [ ] Esboçar estrutura de filesystem
  - decidir entre `include/fs/` e `main/fs/`
  - criar layout de arquivos e operações mínimas
- [ ] Adicionar documentação de arquitetura de memória e processo
  - `documentation/NOTAS.md` ou novo arquivo curto
- [ ] Refatorar código de módulos e headers para clareza
  - remover dead code e imports orfãos
  - documentar porque `graphics.h` está presente sem implementação concreta

### Entregáveis
- Protótipo de design de filesystem pronto para implementação
- Documentação de arquitetura disponível
- Código mais limpo e organizado

---

## Semana 6: Metas de refinamento e validação

### Objetivo principal
Revisar o que foi implementado, corrigir bugs e preparar para a próxima fase.

### Metas
- [ ] Testar a inicialização no QEMU com logs mínimos
- [ ] Verificar se processos conseguem ser criados e desertados
- [ ] Confirmar handlers de interrupção e exceção
- [ ] Validar shell e comandos básicos
- [ ] Atualizar `documentation/roadmap-3.md` com novos passos reais após a validação

### Entregáveis
- Checklist de estabilidade completo para itens das semanas anteriores
- Lista de bugs corrigidos e próximos alvos para fase 2

---

## Como usar este roadmap semanal

- Priorize cada semana com entregáveis claros.
- Se precisar, divida cada meta em tarefas menores no seu sistema de controle.
- Foco inicial: kernel estável e shell funcional.
- Depois: expandir memória, vídeo e filesystem.

---

## Materiais de apoio rápido

- OSDev Wiki: https://wiki.osdev.org/
- "Operating Systems: Three Easy Pieces"
- xv6 MIT (para comparar scheduler e processos)
- VESA/VBE programming guides
- Exemplos de `libc` minimalista: `newlib`, `musl`, `klib`

---

## Observações finais

Este roadmap é projetado para seis semanas de trabalho incremental. Se alguma semana avançar mais rápido, mova imediatamente a próxima meta para frente e mantenha o foco na estabilidade do kernel antes de adicionar recursos maiores.

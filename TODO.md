# TODO - ImagineOS Development Roadmap

**Versão:** vR1 "Jessica"  
**Status:** Em Construção (Instável)  
**Última Atualização:** 2026-06-07

---

## 📋 Índice

1. [Visão Geral do Projeto](#-visão-geral-do-projeto)
2. [Problemas Críticos Identificados](#-problemas-críticos-identificados)
3. [Features Necessárias por Módulo](#-features-necessárias-por-módulo)
4. [Roadmap de Desenvolvimento](#-roadmap-de-desenvolvimento)
5. [Possíveis Bugs](#-possíveis-bugs)
6. [Sugestões de Implementação](#-sugestões-de-implementação)

---

## 🎯 Visão Geral do Projeto

**ImagineOS** é um sistema operacional x86_64 em desenvolvimento para fins educacionais e experimentais. 

### Componentes Principais:
- **Bootloader:** GRUB
- **Kernel:** C + Assembly (x86_64)
- **Shell:** QBshell (linguagem de script customizada)
- **Arquitetura:** x86_64 (32-bit legacy boot)
- **Tipo de Execução:** Bare-metal + QEMU emulation

### Status Atual:
- ✅ Boot sequence básico funcionando
- ✅ Tratamento de interrupts (IDT)
- ✅ Controlador de PIC remapeado
- ✅ Scheduler round-robin simples
- ✅ Gerenciamento básico de processos
- ✅ Entrada de teclado (PS/2)
- ✅ Modo texto VGA
- ❌ Sistema de arquivos
- ❌ Paging/Memória virtual
- ❌ Memory management avançado
- ❌ Modo gráfico (removido, precisa de reimplementação)
- ❌ Drivers de hardware completos

---

## 🚨 Problemas Críticos Identificados

### 1. **Memory Management Deficiente**
- **Problema:** Alocação de memória é fixa e hardcoded
  - Processos criados em `0x100000` (endereço fixo)
  - Sem malloc/free dinâmico
  - Sem heap management
  - Sem paging/TLB
  
- **Impacto:** Alto - Sistema não escalável, impossível criar múltiplos processos
- **Localização:** [main/management/process.c](main/management/process.c#L30)
- **Solução Sugerida:** Implementar:
  1. Heap manager com bitmap/linked list
  2. Virtual memory com paging
  3. Estrutura de page tables

### 2. **Scheduler Incompleto**
- **Problema:** Round-robin muito simplista
  - Sem prioridades
  - Sem preempção adequada
  - Sem sincronização de processos
  - Sem mutexes/semaphores
  
- **Impacto:** Médio - Múltiplos processos podem ter race conditions
- **Localização:** [main/management/scheduler.c](main/management/scheduler.c)
- **Solução Sugerida:**
  1. Implementar preempção com timer interrupt
  2. Adicionar sistema de prioridades
  3. Implementar primitivas de sincronização (mutex, semaphore)

### 3. **Drivers de Hardware Faltando**
- **Problema:** Suporte limitado para hardware
  - PS/2 keyboard: parcialmente implementado
  - Modo gráfico removido (sem implementação atual)
  - Sem driver de mouse
  - Sem driver de disco (IDE/SATA)
  - Sem driver de rede
  - Sem driver de som
  
- **Impacto:** Alto - Limitação significativa de funcionalidade
- **Localização:** [include/devices/](include/devices/), [main/devices/](main/devices/)
- **Solução Sugerida:**
  1. Restaurar driver gráfico VGA
  2. Implementar driver de mouse
  3. Adicionar suporte a AHCI/IDE
  4. Implementar driver de rede básico

### 4. **Modo Gráfico Removido**
- **Problema:** Arquivo `graphics.h` foi removido, código comentado
  - Sistema está em modo texto apenas
  - Font bitmap iniciado mas incompleto
  - Framebuffer não está funcionando
  
- **Impacto:** Médio - Limita UI/experiência
- **Localização:** Comentários em [main/devices/print.c](main/devices/print.c#L4)
- **Solução Sugerida:**
  1. Completar bitmap da fonte (256 caracteres)
  2. Implementar framebuffer manager
  3. Adicionar suporte a modo VESA/UEFI
  4. Considerar UEFI boot (mencionado em NOTAS.md)

### 5. **Sistema de Arquivos Ausente**
- **Problema:** Não há sistema de arquivos
  - Sem persistência de dados
  - Sem I/O de disco
  - Sem hierarquia de diretórios
  
- **Impacto:** Crítico - Impossível salvar dados
- **Localização:** N/A (Não implementado)
- **Solução Sugerida:**
  1. Implementar FAT32 (simples e compatível)
  2. Alternativa: Ext2 (mais robusto)
  3. Prototipagem com RAM disk primeiro

### 6. **Shell Incompleto**
- **Problema:** QBshell tem funcionalidades parciais
  - Sem interpretador de comandos completo
  - Sem redirecionamento (pipes, >, <)
  - Sem suporte a scripts
  - Sem histórico de comandos
  - Sem variáveis de ambiente
  
- **Impacto:** Médio - Experiência de usuário limitada
- **Localização:** [userspace/QBshell/shell.c](userspace/QBshell/shell.c)
- **Solução Sugerida:**
  1. Implementar lexer/parser básico
  2. Adicionar suporte a redirecionamento
  3. Implementar histórico com buffer
  4. Adicionar variáveis de ambiente

### 7. **Code Removido e Incompleto**
- **Problema:** Várias áreas têm código comentado ou removido
  - `graphics.h` foi deletado
  - `main.asm` comentado
  - `math.h` está vazio
  - Referências órfãs em imports
  
- **Impacto:** Baixo - Ruído no código, fácil de limpar
- **Localização:** Vários (grep needed)
- **Solução Sugerida:**
  1. Limpar imports comentados
  2. Implementar math.h
  3. Documentar por que foi removido

### 8. **Tratamento de Erros e Exceções**
- **Problema:** Falta tratamento robusto de erros
  - Sem exceções caught
  - Sem pagefault handler
  - Sem general protection fault handler
  - Sem panic/kernel crash dump
  
- **Impacto:** Alto - Sistema pode travar silenciosamente
- **Localização:** [main/devices/idt.c](main/devices/idt.c)
- **Solução Sugerida:**
  1. Implementar exception handlers (0x00-0x1F)
  2. Adicionar panic function com reboot
  3. Implementar basic error logging

---

## 🔧 Features Necessárias por Módulo

### A. BOOTLOADER & BOOT SEQUENCE

#### Status Atual:
- ✅ GRUB Legacy boot funciona
- ❌ UEFI boot não suportado
- ❌ Sem persistent configuration

#### Features Necessárias:
```
□ UEFI bootloader support
  └─ Necessário para máquinas modernas (BIOS legacy deprecated)
  
□ Boot menu com seleção de opções
  └─ Quiet boot
  └─ Debug mode
  └─ Safe mode

□ Boot configuration file (bootloader.cfg)
  └─ Definições de timeout
  └─ Opções de boot padrão
  └─ Debug logging level

□ Multiboot2 standard compliance
  └─ Melhor compatibilidade com hypervisors
  └─ Suporte a múltiplos módulos
```

### B. MEMORY MANAGEMENT

#### Status Atual:
- ❌ Nenhum memory manager implementado
- ❌ Sem malloc/free
- ❌ Endereços hardcoded

#### Features Necessárias:
```
□ Heap Manager (ALTA PRIORIDADE)
  ├─ Bitmap allocator (simples, rápido)
  │  └─ Estrutura: bitmap de blocks, cada bit = 1 block (4KB)
  │  └─ Funções: kalloc(size), kfree(ptr)
  │  └─ Implementação em: include/management/memory.h
  │
  ├─ Alternative: Linked List Allocator
  │  └─ Estrutura: free list com metadata
  │  └─ First-fit ou Best-fit strategy
  │  
  └─ Tests needed
     └─ Fragmentação
     └─ Memory leaks detection
     └─ Stress test (milhares de alloc/free)

□ Virtual Memory / Paging (ALTA PRIORIDADE)
  ├─ Page Table Management
  │  ├─ 4KB pages
  │  ├─ 4-level page tables (PML4, PDPT, PDT, PT)
  │  ├─ Lazy paging (on-demand allocation)
  │  └─ TLB invalidation
  │
  ├─ Address Space Management
  │  ├─ User space (0x0000000000000000 - 0x00007FFFFFFFFFFF)
  │  ├─ Kernel space (0xFFFF800000000000 - 0xFFFFFFFFFFFFFFFF)
  │  ├─ Kernel higher half (preferido moderno)
  │  └─ Reserve kernel space
  │
  └─ Map kernel to higher half
     └─ Identity mapping para boot

□ Protection & Privilege Levels
  ├─ User mode vs Kernel mode
  ├─ Access control bits (R/W/X)
  └─ Supervisor bit enforcement

□ Memory Layout Documentation
  ├─ Criar arquivo: docs/memory_layout.md
  ├─ Visão geral da divisão de memória
  ├─ Símbolos do linker (kernel start/end)
  └─ Exemplos de alocação
```

**Implementação Recomendada:**
```c
// Em include/management/memory.h
typedef struct {
    void* start;
    size_t size;
    uint8_t* bitmap;
} HeapManager;

void* kalloc(size_t size);
void kfree(void* ptr);
void memory_init();
```

### C. PROCESS & SCHEDULER

#### Status Atual:
- ✅ Processos podem ser criados
- ✅ Round-robin scheduling básico
- ❌ Sem sincronização
- ❌ Sem signal handling

#### Features Necessárias:
```
□ Process Management Improvements
  ├─ Process Table (com limite configurável)
  ├─ Process states: READY, RUNNING, BLOCKED, ZOMBIE, TERMINATED
  ├─ Process termination cleanup
  ├─ Parent-child relationship
  ├─ Process group/session management
  └─ Wait for process (waitpid)

□ Synchronization Primitives (ALTA PRIORIDADE)
  ├─ Mutex (para mutual exclusion)
  │  ├─ Lock/Unlock
  │  ├─ Trylock com timeout
  │  └─ Deadlock detection
  │
  ├─ Semaphore (contador de recursos)
  │  ├─ Wait (P)
  │  ├─ Signal (V)
  │  └─ Binary vs Counting
  │
  ├─ Condition Variables
  │  ├─ Wait/Signal/Broadcast
  │  └─ Para event-driven programming
  │
  └─ Barriers
     └─ Para sincronização de múltiplos threads

□ Signal Handling
  ├─ Signal delivery
  ├─ Signal handlers user-space
  ├─ SIGTERM, SIGKILL, SIGSTOP, SIGCONT
  ├─ SIGSEGV, SIGABRT, SIGFPE
  └─ Signal masking

□ Advanced Scheduling
  ├─ Process priorities (0-39, como Linux)
  ├─ CFS (Completely Fair Scheduler) ou similar
  ├─ CPU affinity (multicore future)
  ├─ Load balancing
  └─ I/O wait awareness

□ Context Switch Improvements
  ├─ Salvar FPU state (se necessário)
  ├─ Salvar AVX registers (se suportado)
  ├─ Flush TLB per-process
  └─ Performance optimization
```

**Arquivo de Implementação:**
```
include/management/synchronization.h
main/management/synchronization.c
include/management/signal.h
main/management/signal.c
```

### D. INTERRUPTS & EXCEPTIONS

#### Status Atual:
- ✅ IDT configurado
- ✅ PIC remapeado
- ✅ Keyboard interrupt funciona
- ✅ Timer interrupt funciona
- ❌ Sem exception handlers (0x00-0x1F)
- ❌ Sem page fault handling

#### Features Necessárias:
```
□ Exception Handlers (ALTA PRIORIDADE)
  ├─ #DE (Divide Error)
  ├─ #DB (Debug Exception)
  ├─ #BP (Breakpoint)
  ├─ #OF (Overflow)
  ├─ #BR (Bound Range Exceeded)
  ├─ #UD (Invalid Opcode)
  ├─ #NM (Device Not Available)
  ├─ #DF (Double Fault)
  ├─ #TS (Invalid TSS)
  ├─ #NP (Segment Not Present)
  ├─ #SS (Stack-Segment Fault)
  ├─ #GP (General Protection Fault) ⭐ IMPORTANTE
  ├─ #PF (Page Fault) ⭐ IMPORTANTE para paging
  ├─ #MF (x87 Floating-Point Error)
  ├─ #AC (Alignment Check)
  ├─ #MC (Machine Check)
  └─ #XM (SIMD Floating-Point)

□ Page Fault Handling
  ├─ Registrar handler no IDT
  ├─ Ler CR2 para linear address
  ├─ Ler error code para reason
  ├─ Alocação lazy de página
  ├─ Copy-on-write (CoW)
  └─ Segmentation fault detection

□ Panic & Error Reporting
  ├─ panic(const char* msg)
  │  ├─ Printa registros
  │  ├─ Stack dump
  │  ├─ Freezes system
  │  └─ Reboota (opcional)
  │
  ├─ Kernel log buffer
  │  ├─ Ring buffer (4KB-64KB)
  │  ├─ Timestamps
  │  └─ Log levels (DEBUG, INFO, WARN, ERROR)
  │
  └─ Crash dump file (para futuro: filesystem)
     └─ Registers snapshot
     └─ Stack trace
     └─ Memory dump

□ APIC Support (para multicore, futuro)
  ├─ LAPIC (Local APIC)
  ├─ I/O APIC
  ├─ IPI (Inter-Processor Interrupt)
  └─ MSI (Message Signaled Interrupts)
```

### E. KEYBOARD & INPUT

#### Status Atual:
- ✅ PS/2 keyboard básico
- ✅ Scancode translation
- ❌ Sem mouse support
- ❌ Sem repeat rate customization

#### Features Necessárias:
```
□ Keyboard Improvements
  ├─ Todos os scancodes mapeados
  ├─ Multiple keyboard layouts
  │  ├─ QWERTY (EN-US, BR, etc)
  │  ├─ AZERTY (FR)
  │  ├─ QWERTZ (DE)
  │  └─ Dvorak
  │
  ├─ Dead keys support (acentos)
  ├─ Repeat rate e delay configuráveis
  ├─ LED feedback (Caps Lock, Num Lock, Scroll Lock)
  └─ Keyboard reset/self-test

□ Mouse Support
  ├─ PS/2 mouse driver
  ├─ Mouse cursor rendering
  ├─ Click detection
  ├─ Wheel support
  └─ USB mouse (futuro)

□ Input Events Subsystem
  ├─ Event queue (ringbuffer)
  ├─ Event types: KEY_PRESS, KEY_RELEASE, MOUSE_MOVE, MOUSE_CLICK
  ├─ Raw input vs cooked input
  └─ Input multiplexing (múltiplos consumidores)

□ Terminal Input Mode
  ├─ Raw mode vs canonical mode
  ├─ Echo control
  ├─ Signal generation (Ctrl+C, Ctrl+Z)
  └─ Line editing (backspace, delete, etc)
```

### F. VIDEO & GRAPHICS

#### Status Atual:
- ✅ Modo texto VGA funcionando
- ❌ Modo gráfico removido
- ❌ Sem suporte a múltiplas resoluções
- ❌ Sem framebuffer access

#### Features Necessárias:
```
□ Graphics Driver Restoration (ALTA PRIORIDADE)
  ├─ Restaurar graphics.h (atualmente deletado)
  ├─ Framebuffer management
  ├─ VGA modes suportados
  │  ├─ 320x200x8bpp (mode 13h)
  │  ├─ 640x480x4bpp (VGA)
  │  └─ Text mode 80x25
  │
  └─ Double buffering (anti-flicker)

□ Text Mode Improvements
  ├─ Cores extendidas (16 cores vs 8)
  ├─ Scrolling regions
  ├─ Cursor styles (block, underline, hidden)
  ├─ Character blink support
  └─ Unicode/UTF-8 display (limitado)

□ Graphical Rendering
  ├─ Primitive drawing
  │  ├─ Point plotting
  │  ├─ Line drawing (Bresenham)
  │  ├─ Rectangle fill
  │  ├─ Circle drawing
  │  └─ Polygon fill
  │
  ├─ Font rendering
  │  ├─ Bitmap fonts (8x16) - já iniciado
  │  ├─ Suporte a múltiplos tamanhos
  │  ├─ Anti-aliasing (nice to have)
  │  └─ Completar font data para 256 caracteres
  │
  ├─ GUI Elements (futuro)
  │  ├─ Windows
  │  ├─ Buttons
  │  ├─ Text input fields
  │  └─ Menus
  │
  └─ Graphics Library (libgfx)
     ├─ High-level drawing API
     └─ Demo programs

□ VESA/UEFI Graphics Mode (futuro)
  ├─ VESA BIOS Extensions (VBE)
  ├─ Múltiplas resoluções
  ├─ Higher color depth (24-bit, 32-bit)
  └─ Linear framebuffer

□ Terminal Emulator (futuro)
  ├─ ANSI/VT100 escape sequences
  ├─ Color support (256-color, true color)
  ├─ Cursor positioning
  └─ Text attributes (bold, underline, etc)
```

### G. DEVICES & DRIVERS

#### Status Atual:
- ✅ Port I/O básico
- ✅ RTC (Real-Time Clock) implementado
- ✅ PIC (Programmable Interrupt Controller)
- ✅ IDT (Interrupt Descriptor Table)
- ✅ GDT (Global Descriptor Table)
- ❌ Sem IOMMU
- ❌ Sem hot-plugging

#### Features Necessárias:
```
□ Serial Port Driver
  ├─ COM1-COM4 support
  ├─ Baud rates configuráveis
  ├─ Debugging console output
  └─ Early kernel boot messages

□ Parallel Port Driver
  ├─ Print device detection
  ├─ Bidirectional mode support
  └─ (Provavelmente obsoleto, considerar remover)

□ IDE/SATA Disk Driver (ALTA PRIORIDADE)
  ├─ IDE hard drive support
  ├─ AHCI (Advanced Host Controller Interface)
  ├─ DMA transfers
  ├─ Command queue
  ├─ Error handling & retry
  └─ Future: NVMe support

□ USB Host Controller Support (futuro)
  ├─ EHCI (Enhanced Host Controller Interface)
  ├─ XHCI (eXtensible Host Controller Interface)
  ├─ USB device enumeration
  ├─ Hub support
  └─ Device driver registration

□ Device Manager
  ├─ Device discovery (PCI enumeration)
  ├─ Device tree
  ├─ Driver loading/unloading
  ├─ Hotplug support
  └─ /dev filesystem interface

□ PCI Enumeration (ALTA PRIORIDADE)
  ├─ PCI Configuration Space access
  ├─ Device scanning
  ├─ Vendor/Device ID lookup
  ├─ IRQ routing
  ├─ MSI/MSI-X support (futuro)
  └─ Pass-through virtualization (futuro)
```

### H. FILE SYSTEM

#### Status Atual:
- ❌ Nenhum sistema de arquivos
- ❌ Sem persistência de dados

#### Features Necessárias:
```
□ VFS (Virtual File System) Abstraction (ALTA PRIORIDADE)
  ├─ inode structure
  ├─ dentry cache
  ├─ File object representation
  ├─ Mount points
  ├─ Filesystem plugins
  └─ Generic file operations (open, read, write, close)

□ FAT32 Filesystem (RECOMENDADO - simples)
  ├─ Read-only initially
  ├─ Directory listing
  ├─ File reading
  ├─ File creation (write support)
  ├─ File deletion
  ├─ Free space management
  ├─ Long filename support (LFN)
  └─ Journaling (nice to have)

□ Alternative: Ext2 Filesystem
  ├─ More modern than FAT32
  ├─ Better for large files
  ├─ Inode-based
  ├─ Symlinks support
  ├─ Permissions & ownership
  └─ Block groups

□ RAM Disk (Prototyping)
  ├─ Virtual block device in RAM
  ├─ For testing filesystem code
  ├─ Faster than real disk
  └─ Rápido para desenvolvimento

□ Disk Cache / Buffer Cache
  ├─ Page cache integration
  ├─ Read-ahead optimization
  ├─ Write-back caching
  ├─ Cache coherency
  └─ LRU eviction

□ File I/O System Calls (POSIX-like)
  ├─ open(path, flags, mode)
  ├─ read(fd, buffer, count)
  ├─ write(fd, buffer, count)
  ├─ close(fd)
  ├─ seek(fd, offset, whence)
  ├─ stat(path)
  ├─ mkdir(path)
  ├─ unlink(path)
  ├─ link(oldpath, newpath)
  └─ symlink(oldpath, newpath)

□ Directory Structure & Initialization
  ├─ Root directory (/)
  ├─ /bin - executables
  ├─ /lib - shared libraries
  ├─ /etc - configuration
  ├─ /tmp - temporary files
  ├─ /home - user home directories
  ├─ /var - variable data
  ├─ /dev - device files
  └─ /sys - system information
```

### I. SHELL & USER INTERFACE

#### Status Atual:
- ✅ Shell básico funciona
- ✅ Alguns comandos implementados
- ❌ Sem interpretador completo
- ❌ Sem suporte a pipes
- ❌ Sem redirecionamento

#### Features Necessárias:
```
□ Command Parser & Interpreter (ALTA PRIORIDADE)
  ├─ Lexer/Tokenizer
  ├─ Parser de sintaxe
  ├─ Command execution
  ├─ Error handling com exit codes
  ├─ Pipeline support (cmd1 | cmd2)
  ├─ Redirecionamento
  │  ├─ Input: cmd < file
  │  ├─ Output: cmd > file
  │  ├─ Append: cmd >> file
  │  └─ Error: cmd 2> file
  │
  ├─ Background execution (cmd &)
  ├─ Command substitution $(cmd)
  ├─ Variable expansion $VAR
  ├─ Glob patterns (*, ?, [])
  └─ Quotes handling (single, double, backticks)

□ Built-in Commands (ALTA PRIORIDADE)
  ├─ Navigation
  │  ├─ cd - change directory
  │  ├─ pwd - print working directory
  │  ├─ ls - list files
  │  └─ tree - directory tree
  │
  ├─ File Operations
  │  ├─ cat - concatenate/view files
  │  ├─ cp - copy files
  │  ├─ mv - move/rename files
  │  ├─ rm - remove files
  │  ├─ mkdir - create directory
  │  ├─ rmdir - remove directory
  │  ├─ touch - create/update file
  │  └─ file - determine file type
  │
  ├─ Process Management
  │  ├─ ps - list processes
  │  ├─ kill - terminate process
  │  ├─ bg - resume in background
  │  ├─ fg - resume in foreground
  │  ├─ jobs - list background jobs
  │  └─ wait - wait for process
  │
  ├─ System Information
  │  ├─ uname - system information
  │  ├─ whoami - current user
  │  ├─ id - user/group id
  │  ├─ date - date/time
  │  ├─ free - memory info
  │  ├─ df - disk space
  │  ├─ uptime - system uptime
  │  └─ dmesg - kernel log
  │
  ├─ Text Processing
  │  ├─ echo - print text
  │  ├─ grep - pattern search
  │  ├─ sed - stream editor
  │  ├─ awk - text processing
  │  ├─ cut - column extraction
  │  ├─ sort - sort lines
  │  ├─ uniq - unique lines
  │  ├─ wc - word/line count
  │  ├─ head - first lines
  │  └─ tail - last lines
  │
  ├─ Development
  │  ├─ gcc/clang - compilation
  │  ├─ make - build automation
  │  ├─ ar - archive manager
  │  ├─ nm - symbol listing
  │  └─ objdump - disassembler
  │
  ├─ System Control
  │  ├─ shutdown - power off
  │  ├─ reboot - restart
  │  ├─ halt - stop CPU
  │  └─ sync - flush caches
  │
  └─ Misc
     ├─ help - command help
     ├─ clear - clear screen
     ├─ history - command history
     ├─ alias - command alias
     ├─ export - environment variable
     ├─ set - set options
     └─ exit - exit shell

□ Shell Features
  ├─ Command history (up/down arrows)
  ├─ Line editing (Emacs-like shortcuts)
  │  ├─ Ctrl+A - beginning of line
  │  ├─ Ctrl+E - end of line
  │  ├─ Ctrl+L - clear screen
  │  ├─ Ctrl+U - kill line
  │  └─ Ctrl+R - reverse search
  │
  ├─ Tab completion
  │  ├─ File path completion
  │  ├─ Command completion
  │  └─ History completion
  │
  ├─ Environment variables
  │  ├─ PATH - executable search path
  │  ├─ HOME - user home directory
  │  ├─ USER - current user
  │  ├─ PWD - current directory
  │  ├─ SHELL - shell program
  │  ├─ TERM - terminal type
  │  └─ Custom user variables
  │
  ├─ Alias support
  ├─ Configuration file (.bashrc equivalent)
  ├─ Script execution from files
  ├─ Shell scripting language
  └─ Signal handling (Ctrl+C, Ctrl+Z)

□ Shell Scripting Language (QBScript)
  ├─ Variables
  ├─ Loops (for, while, do-while)
  ├─ Conditionals (if-else, case)
  ├─ Functions
  ├─ String manipulation
  ├─ Array support
  ├─ File I/O
  ├─ Pattern matching
  └─ Error handling (try-catch)

□ Terminal Multiplexer (futuro, como tmux)
  ├─ Multiple windows
  ├─ Panes/splits
  ├─ Window management
  ├─ Session persistence
  └─ Clipboard integration
```

### J. STANDARD LIBRARY

#### Status Atual:
- ✅ string.h com funções básicas
- ✅ stdio.h com wrappers
- ✅ bool.h com tipos
- ❌ math.h vazio
- ❌ Sem stdlib (malloc, etc)
- ❌ Sem time.h

#### Features Necessárias:
```
□ Standard Headers Implementation

  A. math.h (URGENTE - VAZIO)
    ├─ Basic arithmetic
    │  ├─ abs, fabs, div
    │  ├─ sqrt, pow, exp, log
    │  ├─ sin, cos, tan
    │  ├─ asin, acos, atan
    │  ├─ sinh, cosh, tanh
    │  └─ floor, ceil, round, trunc
    │
    ├─ Constants
    │  ├─ M_PI
    │  ├─ M_E
    │  ├─ M_LN2, M_LN10
    │  └─ M_SQRT2
    │
    ├─ Macros
    │  ├─ fmax, fmin
    │  ├─ isnan, isinf, isfinite
    │  └─ fpclassify
    │
    └─ Test suite para validar

  B. stdlib.h
    ├─ Memory allocation
    │  ├─ malloc, calloc, realloc, free
    │  ├─ aligned_alloc
    │  └─ Leaks detection
    │
    ├─ Random number generation
    │  ├─ rand(), srand()
    │  ├─ random(), srandom()
    │  └─ arc4random() (melhor qualidade)
    │
    ├─ String conversion
    │  ├─ atoi, atof, atol
    │  ├─ strtol, strtof, strtod
    │  └─ itoa, ftoa (inverse)
    │
    ├─ qsort - sorting
    ├─ bsearch - binary search
    ├─ abs, div, ldiv
    ├─ atexit - registration
    └─ exit, abort, system

  C. string.h (incompleto, expandir)
    ├─ Completar implementação
    ├─ strcpy, strncpy, strcat, strncat
    ├─ memcpy, memmove, memset, memcmp
    ├─ strcspn, strspn, strpbrk, strtok
    ├─ strstr, strrchr, strsep
    ├─ strerror
    └─ String case functions

  D. stdio.h (wrappers, expandir)
    ├─ printf, sprintf, snprintf (parsing)
    ├─ scanf, sscanf (parsing)
    ├─ getchar, putchar, gets, puts
    ├─ fgets, fputs (com file handles)
    ├─ FILE* operations
    ├─ fopen, fclose, fread, fwrite
    ├─ fseek, ftell, rewind
    ├─ stdin, stdout, stderr
    └─ Buffering control

  E. time.h
    ├─ time_t type
    ├─ time() - get current time
    ├─ gmtime() - convert to struct
    ├─ mktime() - reverse
    ├─ strftime() - format
    ├─ clock() - elapsed time
    ├─ difftime() - time difference
    ├─ localtime() - local timezone
    └─ tzname, timezone support

  F. ctype.h
    ├─ Character classification
    │  ├─ isalpha, isdigit, isalnum
    │  ├─ isspace, isupper, islower
    │  ├─ ispunct, iscntrl, isgraph
    │  └─ isxdigit
    │
    └─ Character conversion
       ├─ toupper, tolower
       └─ toascii

  G. assert.h
    ├─ assert() macro
    ├─ static_assert (C11)
    └─ Compile-time checks

  H. limits.h
    ├─ INT_MIN, INT_MAX
    ├─ LONG_MIN, LONG_MAX
    ├─ CHAR_MIN, CHAR_MAX
    ├─ FLT_MIN, FLT_MAX, DBL_MIN, DBL_MAX
    └─ PATH_MAX, NAME_MAX, etc

  I. errno.h
    ├─ errno variable
    ├─ Error code constants
    └─ perror() function

  J. stdint.h
    ├─ Fixed-size integer types
    ├─ int8_t, uint8_t, int16_t, uint16_t
    ├─ int32_t, uint32_t, int64_t, uint64_t
    ├─ intptr_t, uintptr_t
    └─ MIN/MAX constants

  K. stddef.h / sys/types.h
    ├─ size_t, ssize_t, ptrdiff_t
    ├─ wchar_t, wchart_t
    ├─ NULL
    ├─ offsetof macro
    └─ Common type definitions

  L. unistd.h (POSIX)
    ├─ POSIX I/O
    │  ├─ open, close, read, write
    │  ├─ lseek, dup, dup2
    │  ├─ pipe
    │  └─ fcntl
    │
    ├─ Process management
    │  ├─ fork, exec, exit
    │  ├─ getpid, getppid
    │  ├─ getuid, getgid
    │  └─ setuid, setgid
    │
    ├─ File system
    │  ├─ chdir, getcwd
    │  ├─ stat, fstat, lstat
    │  ├─ unlink, link, symlink
    │  └─ mkdir, rmdir
    │
    └─ Misc
       ├─ sleep, usleep
       ├─ sysconf
       └─ gethost/getdomain name

  M. signal.h
    ├─ signal() - set handler
    ├─ sigaction() - advanced
    ├─ sigprocmask() - masking
    ├─ pause() - wait for signal
    ├─ kill() - send signal
    ├─ raise() - send to self
    └─ Signal constants

  N. sys/stat.h
    ├─ stat structures
    ├─ File type macros
    ├─ Permission bits
    ├─ S_ISREG, S_ISDIR, S_ISLNK
    └─ chmod, mkdir, rmdir

  O. pthread.h (opcional, para threads)
    ├─ pthread_create
    ├─ pthread_join
    ├─ pthread_mutex_*
    ├─ pthread_cond_*
    ├─ pthread_rwlock_*
    └─ Thread-local storage (TLS)
```

### K. USERSPACE APPLICATIONS

#### Status Atual:
- ✅ calc.c - calculadora
- ✅ clear.c - limpar tela
- ✅ halt.c - halt CPU
- ✅ help.c - help
- ✅ time.c - hora
- ✅ version.c - versão
- ✅ video.c - driver de vídeo

#### Features Necessárias:
```
□ Application Framework
  ├─ Standard entry point convention
  ├─ Argument parsing library
  ├─ Return codes standardization
  ├─ Usage/help text standardization
  ├─ Configuration files
  └─ Plugin system

□ System Utilities
  ├─ ls - list files (não apenas pwd)
  ├─ top - process monitor
  ├─ htop - interactive top
  ├─ netstat - network statistics
  ├─ ps - process listing
  ├─ mount/umount - filesystem mounting
  ├─ fsck - filesystem check
  ├─ dd - data duplicator
  ├─ tar - archive tool
  ├─ gzip/bzip2 - compression
  └─ diff/patch - file differences

□ Text Editors
  ├─ nano-like editor (learning project)
  ├─ vi/vim-like editor (advanced)
  └─ Emacs-like editor (optional)

□ Development Tools
  ├─ cc/gcc wrapper
  ├─ ld linker wrapper
  ├─ as assembler wrapper
  ├─ make build tool
  ├─ gdb-like debugger
  ├─ strace equivalent
  └─ valgrind-like memory tool

□ Games/Entertainment
  ├─ 2048
  ├─ Snake
  ├─ Tetris
  ├─ Roguelike dungeon crawler
  └─ Educational games

□ Utilities
  ├─ bc - calculator (better than calc)
  ├─ Lua interpreter (mencionado em TODO QBshell)
  ├─ Python interpreter (futuro)
  ├─ IRC client (futuro)
  ├─ HTTP server (futuro)
  ├─ SSH client/server (futuro)
  └─ Database (futuro)
```

---

## 🗓️ Roadmap de Desenvolvimento

### PHASE 0: Foundation & Infrastructure (CURRENT - v1.0)
**Duração Estimada:** 2-3 meses

```
v0.1 - Core Kernel
├─ [x] Boot sequence
├─ [x] IDT/GDT/PIC setup
├─ [x] Basic keyboard input
├─ [x] Timer interrupts
├─ [x] Process creation (básico)
├─ [x] Round-robin scheduler
├─ [ ] Memory allocator (kalloc/kfree)
├─ [ ] Exception handlers (GP, PF)
├─ [ ] Kernel panic/logging
└─ [ ] Documentation update

v0.2 - Memory Management
├─ [ ] Heap manager implementation
├─ [ ] Paging enable
├─ [ ] Higher half kernel mapping
├─ [ ] Page fault handling
├─ [ ] Memory protection bits
└─ [ ] Performance testing

v0.3 - Process & Synchronization
├─ [ ] Mutex implementation
├─ [ ] Semaphore implementation
├─ [ ] Signal handling basics
├─ [ ] Process termination cleanup
├─ [ ] Process state transitions
└─ [ ] Deadlock detection (basic)

v0.4 - Shell & User Interface
├─ [ ] Command parser
├─ [ ] Basic built-in commands
├─ [ ] Command history
├─ [ ] Pipes (cmd1 | cmd2)
├─ [ ] Redirection (>, <, >>)
└─ [ ] Tab completion (basic)
```

### PHASE 1: Hardware Support (v1.1-1.2)
**Duração Estimada:** 3-4 meses

```
v1.1 - Graphics Restoration
├─ [ ] Restore graphics.h (restaurar code deletado)
├─ [ ] VGA framebuffer support
├─ [ ] Primitive drawing (lines, circles, rects)
├─ [ ] Font rendering (completar 256 chars)
├─ [ ] Double buffering
├─ [ ] GUI framework (windows, buttons)
└─ [ ] Demo applications

v1.2 - Device Drivers
├─ [ ] Serial port driver (COM1-4)
├─ [ ] IDE/SATA disk driver
├─ [ ] PCI enumeration
├─ [ ] USB EHCI support
├─ [ ] Device manager framework
├─ [ ] Hotplug detection
└─ [ ] /dev filesystem

v1.3 - Input Enhancement
├─ [ ] Mouse PS/2 support
├─ [ ] Keyboard layouts (PT-BR, EN-US, FR, DE)
├─ [ ] Dead keys & accents
├─ [ ] Input event system
└─ [ ] Terminal raw mode
```

### PHASE 2: Filesystem (v2.0-2.1)
**Duração Estimada:** 2-3 meses

```
v2.0 - VFS & FAT32
├─ [ ] Virtual File System layer
├─ [ ] FAT32 read support
├─ [ ] FAT32 write support
├─ [ ] Directory listing
├─ [ ] File creation/deletion
├─ [ ] Long filename (LFN) support
├─ [ ] Basic permissions (read-only initially)
└─ [ ] Stress testing

v2.1 - Filesystem Integration
├─ [ ] Mount points
├─ [ ] Path resolution
├─ [ ] Symlink support
├─ [ ] Hard link support
├─ [ ] Buffer cache
├─ [ ] Read-ahead optimization
└─ [ ] Journaling (nice to have)
```

### PHASE 3: POSIX Compliance (v3.0-3.1)
**Duração Estimada:** 3-4 meses

```
v3.0 - System Calls
├─ [ ] open/close syscalls
├─ [ ] read/write syscalls
├─ [ ] fork syscall
├─ [ ] exec* syscalls
├─ [ ] wait/waitpid syscalls
├─ [ ] exit syscall
├─ [ ] stat/fstat syscalls
├─ [ ] ioctl syscall (genérico)
├─ [ ] signal/sigaction syscalls
├─ [ ] mmap/munmap syscalls (memory mapping)
└─ [ ] Syscall ABI specification

v3.1 - Shell Enhancement
├─ [ ] Script execution (from filesystem)
├─ [ ] Variable expansion
├─ [ ] Command substitution $()
├─ [ ] Glob patterns (*, ?, [])
├─ [ ] Background jobs management
├─ [ ] Foreground/background control
├─ [ ] .profile/.shellrc support
└─ [ ] Environment variables
```

### PHASE 4: Standard Library (v4.0-4.1)
**Duração Estimada:** 2 meses

```
v4.0 - libc Completion
├─ [ ] math.h - complete
├─ [ ] stdlib.h - complete
├─ [ ] stdio.h - complete (with files)
├─ [ ] string.h - complete
├─ [ ] time.h - complete
├─ [ ] ctype.h - complete
├─ [ ] assert.h - complete
└─ [ ] Comprehensive testing

v4.1 - POSIX Headers
├─ [ ] unistd.h
├─ [ ] sys/stat.h
├─ [ ] signal.h
├─ [ ] sys/types.h
├─ [ ] fcntl.h
├─ [ ] errno.h
├─ [ ] stdint.h
└─ [ ] sys/ioctl.h
```

### PHASE 5: Development Tools (v5.0)
**Duração Estimada:** 2 meses

```
v5.0 - Compiler Toolchain
├─ [ ] GCC porting
├─ [ ] Binutils porting (as, ld, etc)
├─ [ ] Make utility
├─ [ ] GDB debugger basic port
├─ [ ] Archive tools (ar, ranlib)
├─ [ ] Disassembler (objdump)
└─ [ ] Native compilation (self-hosting)
```

### PHASE 6: System Utilities (v6.0)
**Duração Estimada:** 1 mês

```
v6.0 - Core Utilities
├─ [ ] ls with detailed output
├─ [ ] cat with multiple files
├─ [ ] grep with regex
├─ [ ] sed stream editor
├─ [ ] awk text processor
├─ [ ] find file search
├─ [ ] tar archive
├─ [ ] gzip compression
└─ [ ] diff/patch utilities
```

### PHASE 7: Production Release (v7.0)
**Duração Estimada:** 1 mês

```
v7.0 - Release Preparation
├─ [ ] Bug fixes & optimization
├─ [ ] Documentation complete
├─ [ ] User manual
├─ [ ] Developer guide
├─ [ ] Build system refinement
├─ [ ] CI/CD setup (GitHub Actions)
├─ [ ] Release notes
└─ [ ] Marketing materials

RELEASE: ImagineOS vR2 "Release Candidate"
├─ Bootable ISO
├─ Installable to USB
├─ QEMU ready
├─ VirtualBox ready
└─ Documentation complete
```

---

## 🐛 Possíveis Bugs

### Critical Bugs

#### 1. Memory Overflow in Keyboard Input
- **File:** [main/devices/keyboard.c](main/devices/keyboard.c)
- **Issue:** Scancode mapping pode causar buffer overflow se array bounds não forem verificados
- **Código:**
  ```c
  uint16_t fat_code = (uint16_t)(scan_code & 0x7F);
  // Sem bounds check em array de mapeamento!
  ```
- **Fix:** Adicionar bounds check antes de acessar arrays de scancodes
- **Prioridade:** 🔴 CRÍTICA

#### 2. Process Stack Overflow
- **File:** [main/management/process.c](main/management/process.c#L30)
- **Issue:** Alocação de pilha é fixa (4KB), sem proteção de stack overflow
- **Código:**
  ```c
  #define PROCESS_STACK_SIZE 4096
  // Sem guard pages!
  ```
- **Fix:** Adicionar guard page após cada stack
- **Prioridade:** 🔴 CRÍTICA

#### 3. Hardcoded Memory Address
- **File:** [main/management/process.c](main/management/process.c#L31)
- **Issue:** Todos os processos alocados no mesmo endereço fixo `0x100000`
- **Código:**
  ```c
  Process* proc = (Process*) 0x100000;  // Todos aqui!!!
  ```
- **Fix:** Implementar heap manager dinâmico
- **Prioridade:** 🔴 CRÍTICA

#### 4. Missing Context Save in Interrupt Handlers
- **File:** [main/devices/idt.c](main/devices/idt.c#L55)
- **Issue:** Context switches podem perder registros se FPU/SSE estão sendo usados
- **Fix:** Completar context save com FPU state
- **Prioridade:** 🟠 ALTA

#### 5. Race Condition in Scheduler
- **File:** [main/management/scheduler.c](main/management/scheduler.c#L28)
- **Issue:** Scheduler não tem mutex/lock, múltiplos interrupt handlers podem causar race
- **Código:**
  ```c
  extern Process* process_list;
  // Sem sincronização!
  ```
- **Fix:** Adicionar cli/sti ou mutex
- **Prioridade:** 🟠 ALTA

### High Priority Bugs

#### 6. Incomplete Font Bitmap
- **File:** [main/devices/print.c](main/devices/print.c#L35)
- **Issue:** Font bitmap só tem alguns caracteres, resto pode ter lixo
- **Fix:** Completar todos os 256 caracteres ASCII
- **Prioridade:** 🟡 MÉDIA

#### 7. Missing math.h Implementation
- **File:** [include/libraries/math.h](include/libraries/math.h)
- **Issue:** Arquivo completamente vazio
- **Fix:** Implementar funções matemáticas básicas
- **Prioridade:** 🟡 MÉDIA

#### 8. String Function Edge Cases
- **File:** [include/libraries/string.h](include/libraries/string.h)
- **Issue:** Não há proteção contra NULL pointers em vários strncpy, etc
- **Fix:** Adicionar NULL checks e bounds validation
- **Prioridade:** 🟡 MÉDIA

#### 9. Keyboard Handler NULL Check
- **File:** [main/devices/keyboard.c](main/devices/keyboard.c#L25)
- **Issue:** Verifica se handler é NULL, mas pode ser chamado de múltiplas interrupções
- **Fix:** Adicionar reentrancy protection
- **Prioridade:** 🟡 MÉDIA

#### 10. No Exception Handlers
- **File:** [main/devices/idt.c](main/devices/idt.c)
- **Issue:** Apenas timer e keyboard handlers, nenhum para exceções (GP, PF, DF)
- **Fix:** Implementar handlers para exceções críticas
- **Prioridade:** 🟠 ALTA

### Medium Priority Bugs

#### 11. Memory Leak in modules_load()
- **File:** [main/management/modules.c](main/management/modules.c#L18)
- **Issue:** modules_output() não libera recursos
- **Fix:** Adicionar cleanup logic
- **Prioridade:** 🟡 MÉDIA

#### 12. Incomplete strlen Implementation
- **File:** [include/libraries/string.h](include/libraries/string.h)
- **Issue:** strlen pode não ser thread-safe
- **Fix:** Verificar implementação em string.c
- **Prioridade:** 🟢 BAIXA

#### 13. Graphics Removed But Referenced
- **File:** Múltiplos
- **Issue:** `#include "graphics.h"` comentado, mas arquivo deletado
- **Fix:** Restaurar ou remover referências completamente
- **Prioridade:** 🟡 MÉDIA

---

## 💡 Sugestões de Implementação

### Quick Wins (Fáceis, Alto Impacto)

#### 1. Implementar math.h Completamente
**Dificuldade:** ⭐⭐ (Fácil)  
**Impacto:** Alto (muitas aplicações usam)  
**Tempo Estimado:** 2-4 horas  

```c
// include/libraries/math.h
#pragma once

#include <stdint.h>

#define M_PI        3.14159265358979323846
#define M_E         2.71828182845904523536
#define M_LN2       0.69314718055994530942
#define M_LN10      2.30258509299404568402
#define M_SQRT2     1.41421356237309504880

// Basic arithmetic
double sqrt(double x);
double pow(double x, double y);
double exp(double x);
double log(double x);   // natural log
double log10(double x); // base 10

// Trigonometric
double sin(double x);
double cos(double x);
double tan(double x);
double asin(double x);
double acos(double x);
double atan(double x);
double atan2(double y, double x);

// Hyperbolic
double sinh(double x);
double cosh(double x);
double tanh(double x);

// Rounding
double floor(double x);
double ceil(double x);
double round(double x);
double trunc(double x);
double fabs(double x);

// Min/Max
#define fmax(x, y) ((x) > (y) ? (x) : (y))
#define fmin(x, y) ((x) < (y) ? (x) : (y))

// Classification
int isnan(double x);
int isinf(double x);
int isfinite(double x);
```

**Próximos Passos:**
1. Implementar em `main/libraries/math.c`
2. Usar algoritmos de aproximação (Taylor series para sin/cos, Newton para sqrt, etc)
3. Testar com 100+ casos de teste
4. Documentar em `docs/math_functions.md`

---

#### 2. Limpar Code Removido & Comentado
**Dificuldade:** ⭐ (Trivial)  
**Impacto:** Médio (código mais limpo)  
**Tempo Estimado:** 30 minutos  

```bash
# Commands to find commented code
grep -r "^//" main/ include/  # // comments
grep -r "^#" main/ include/ | grep -v "^#include\|^#define" # Preprocessor

# Check for .h/.c files que podem ser órfãos
find . -name "*.h" -o -name "*.c" | sort
```

**Próximos Passos:**
1. Remover imports comentados
2. Documentar por que graphics.h foi removido em CLEANUP_LOG.md
3. Se necessário, restaurar graphics.h com implementação básica
4. Usar git para rastrear mudanças

---

#### 3. Criar Sistema Básico de Logging do Kernel
**Dificuldade:** ⭐⭐ (Fácil)  
**Impacto:** Alto (debuggable kernel)  
**Tempo Estimado:** 2-3 horas  

```c
// include/management/klog.h
#pragma once

#include <stdint.h>

#define KLOG_DEBUG   0
#define KLOG_INFO    1
#define KLOG_WARN    2
#define KLOG_ERROR   3
#define KLOG_PANIC   4

void klog_init();
void klog(int level, const char* fmt, ...);
void klog_dump();

#define kdebug(fmt, ...) klog(KLOG_DEBUG, fmt, ##__VA_ARGS__)
#define kinfo(fmt, ...)  klog(KLOG_INFO, fmt, ##__VA_ARGS__)
#define kwarn(fmt, ...)  klog(KLOG_WARN, fmt, ##__VA_ARGS__)
#define kerror(fmt, ...) klog(KLOG_ERROR, fmt, ##__VA_ARGS__)
#define kpanic(fmt, ...) klog(KLOG_PANIC, fmt, ##__VA_ARGS__)
```

**Implementação:**
- Ring buffer (4KB) na kernel BSS
- Printf-like formatting
- Timestamps com RTC
- Dumping on panic

---

#### 4. Adicionar Bounds Checking em Arrays
**Dificuldade:** ⭐⭐ (Fácil)  
**Impacto:** Alto (security)  
**Tempo Estimado:** 1 hora  

```c
// Exemplo em keyboard.c
#define KEYBOARD_SCANCODE_MAX 256
#define KEYBOARD_KEYMAP_SIZE 256

// Antes:
char* keymap = scancode_to_ascii[scan_code];  // UNSAFE!

// Depois:
if (scan_code >= KEYBOARD_SCANCODE_MAX) {
    klog_warn("Invalid scancode: 0x%X", scan_code);
    return;
}
char* keymap = scancode_to_ascii[scan_code];  // SAFE
```

---

### Medium Complexity (Moderado, Bom Impacto)

#### 5. Implementar Heap Manager Básico
**Dificuldade:** ⭐⭐⭐ (Moderado)  
**Impacto:** CRÍTICO (soluciona problema #3)  
**Tempo Estimado:** 8-12 horas  

**Estratégia: Bitmap Allocator**

```c
// include/management/memory.h
#pragma once

#include <stddef.h>
#include <stdint.h>

#define HEAP_SIZE        (2 * 1024 * 1024)  // 2MB
#define BLOCK_SIZE       4096                // 4KB
#define NUM_BLOCKS       (HEAP_SIZE / BLOCK_SIZE)
#define BITMAP_SIZE      (NUM_BLOCKS / 8)

typedef struct {
    uint8_t bitmap[BITMAP_SIZE];
    uint8_t heap[HEAP_SIZE];
} Heap;

void heap_init();
void* kalloc(size_t size);
void kfree(void* ptr);
size_t heap_used();
size_t heap_free();
void heap_stats();

// Debug
void heap_dump_bitmap();
void heap_validate();
```

**Implementação em main/management/memory.c:**

```c
static Heap heap;
static bool heap_initialized = false;

void heap_init() {
    memset(&heap, 0, sizeof(Heap));
    heap_initialized = true;
    kinfo("Heap initialized: %u blocks (%u KB)", 
          NUM_BLOCKS, HEAP_SIZE / 1024);
}

void* kalloc(size_t size) {
    // Calcula número de blocks necessários (arredonda pra cima)
    size_t blocks_needed = (size + BLOCK_SIZE - 1) / BLOCK_SIZE;
    
    // Encontra sequência contígua de blocks vazios
    for (size_t i = 0; i + blocks_needed < NUM_BLOCKS; i++) {
        bool found = true;
        for (size_t j = 0; j < blocks_needed; j++) {
            if (heap.bitmap[(i + j) / 8] & (1 << ((i + j) % 8))) {
                found = false;
                break;
            }
        }
        
        if (found) {
            // Marca como alocado
            for (size_t j = 0; j < blocks_needed; j++) {
                heap.bitmap[(i + j) / 8] |= (1 << ((i + j) % 8));
            }
            
            void* ptr = (void*)(&heap.heap[i * BLOCK_SIZE]);
            kdebug("kalloc(%zu) -> %p (blocks %zu-%zu)", 
                   size, ptr, i, i + blocks_needed - 1);
            return ptr;
        }
    }
    
    kerror("kalloc(%zu) FAILED - out of memory", size);
    return NULL;
}

void kfree(void* ptr) {
    if (!ptr) return;
    
    // Calcula block index
    ptrdiff_t offset = (uint8_t*)ptr - (uint8_t*)&heap.heap[0];
    if (offset < 0 || offset >= (ptrdiff_t)HEAP_SIZE) {
        kerror("kfree(%p) - invalid pointer", ptr);
        return;
    }
    
    size_t block_idx = offset / BLOCK_SIZE;
    
    // Limpa o bit (não sabe o tamanho, então limpa só um?)
    // MELHORÍA: Armazenar metadata do tamanho antes de cada alocação
    heap.bitmap[block_idx / 8] &= ~(1 << (block_idx % 8));
    
    kdebug("kfree(%p) block %zu", ptr, block_idx);
}
```

**PROBLEMAS & SOLUÇÕES:**
1. **Problema:** Não sabemos tamanho da alocação no kfree()
   - **Solução:** Adicionar header antes de cada alocação
   
```c
typedef struct {
    size_t size;
    uint32_t magic; // 0xDEADBEEF para validação
} AllocationHeader;

// Modify kalloc:
AllocationHeader* header = (AllocationHeader*)raw_ptr;
header->size = size;
header->magic = 0xDEADBEEF;
return (void*)(header + 1);

// Modify kfree:
AllocationHeader* header = ((AllocationHeader*)ptr) - 1;
if (header->magic != 0xDEADBEEF) {
    panic("kfree: corrupted header!");
}
// Free based on header->size
```

**Próximos Passos:**
1. Criar `main/management/memory.c`
2. Implementar com header
3. Adicionar testes em `tests/memory_test.c`
4. Integrar com `process.c` para usar kalloc ao invés de endereço fixo

---

#### 6. Implementar Exception Handlers Básicos
**Dificuldade:** ⭐⭐⭐ (Moderado)  
**Impacto:** CRÍTICO (impede crashes silenciosos)  
**Tempo Estimado:** 6-8 horas  

```c
// include/devices/exceptions.h
#pragma once

#include <stdint.h>

#define EXC_DIVIDE_ERROR       0x00
#define EXC_DEBUG              0x01
#define EXC_BREAKPOINT         0x03
#define EXC_OVERFLOW           0x04
#define EXC_BOUND_RANGE        0x05
#define EXC_INVALID_OPCODE     0x06
#define EXC_DEVICE_NOT_AVAIL   0x07
#define EXC_DOUBLE_FAULT       0x08
#define EXC_COPROC_OVERRUN     0x09
#define EXC_INVALID_TSS        0x0A
#define EXC_SEGMENT_NOT_PRESENT 0x0B
#define EXC_STACK_FAULT        0x0C
#define EXC_GENERAL_PROTECTION 0x0D  // ⭐ IMPORTANTE
#define EXC_PAGE_FAULT         0x0E  // ⭐ IMPORTANTE
#define EXC_FLOATING_POINT     0x10
#define EXC_ALIGNMENT_CHECK    0x11

typedef struct {
    uint64_t rax, rbx, rcx, rdx;
    uint64_t rsi, rdi, rbp, rsp;
    uint64_t r8, r9, r10, r11, r12, r13, r14, r15;
    uint64_t rip, rflags;
    uint64_t error_code;
    uint64_t exception_number;
} ExceptionFrame;

void exceptions_init();
void exception_handler(ExceptionFrame* frame);
```

**Implementação em main/devices/exceptions.c:**

```c
void exception_handler_gpf(ExceptionFrame* frame) {
    kpanic("GENERAL PROTECTION FAULT at 0x%lX\n"
           "Error Code: 0x%lX\n"
           "CS: 0x%lX\n",
           frame->rip, frame->error_code, 
           frame->rip >> 63 ? 0x18 : 0x08);
    
    // Dump registers
    kdebug("RAX=%lX RBX=%lX RCX=%lX RDX=%lX", 
           frame->rax, frame->rbx, frame->rcx, frame->rdx);
    kdebug("RSI=%lX RDI=%lX RBP=%lX RSP=%lX",
           frame->rsi, frame->rdi, frame->rbp, frame->rsp);
    
    // Reboota
    asm("cli; hlt");
}

void exception_handler_pf(ExceptionFrame* frame) {
    uint64_t faulting_addr;
    asm("movq %%cr2, %0" : "=r"(faulting_addr));
    
    kwarn("PAGE FAULT at 0x%lX (accessed 0x%lX)", 
          frame->rip, faulting_addr);
    
    // Error code bits:
    // 0: Present (0=not present, 1=protection)
    // 1: Write (0=read, 1=write)
    // 2: User (0=kernel, 1=user)
    // 3: Reserved write
    // 4: Instruction fetch
    
    int user = (frame->error_code >> 2) & 1;
    int write = (frame->error_code >> 1) & 1;
    int present = frame->error_code & 1;
    
    kdebug("User=%d Write=%d Present=%d", user, write, present);
    
    if (!present) {
        // Lazy allocation - aloca página
        kdebug("Allocating page at 0x%lX", 
               faulting_addr & ~0xFFF);
        // TODO: Implementar paging
    }
    
    kpanic("PAGE FAULT - unhandled");
}
```

**Handlers em Assembly:**

```asm
; main/devices/exceptions_asm.asm

extern exception_handler
extern exception_handler_gpf
extern exception_handler_pf

; Macro para exception handler com error code
%macro EXCEPTION_HANDLER_ERR 1
exception_handler_%1:
    push rbx
    push r12
    push r13
    push r14
    push r15
    push rbp
    push rdi
    push rsi
    push rdx
    push rcx
    push rax
    
    mov rdi, rsp  ; frame pointer
    call exception_handler_%1_c
    
    pop rax
    pop rcx
    pop rdx
    pop rsi
    pop rdi
    pop rbp
    pop r15
    pop r14
    pop r13
    pop r12
    pop rbx
    add rsp, 8  ; remove error code
    iretq
%endmacro

EXCEPTION_HANDLER_ERR gpf  ; #13
EXCEPTION_HANDLER_ERR pf   ; #14
```

**Integração com IDT:**

```c
// Em idt_init()
void idt_init() {
    // ... existing code ...
    
    // Exception handlers
    idt_set_entry(13, (uint64_t)exception_handler_gpf, 
                  GDT_SELECTOR_CS_KERNEL, IDT_ENTRY_TYPE_INTERRUPT);
    idt_set_entry(14, (uint64_t)exception_handler_pf,
                  GDT_SELECTOR_CS_KERNEL, IDT_ENTRY_TYPE_INTERRUPT);
    
    // ... rest ...
}
```

---

#### 7. Implementar Sincronização de Processos (Mutex)
**Dificuldade:** ⭐⭐⭐ (Moderado)  
**Impacto:** CRÍTICO (race conditions)  
**Tempo Estimado:** 4-6 horas  

```c
// include/management/synchronization.h
#pragma once

#include <stdint.h>

typedef struct {
    volatile uint32_t locked;
    // Futuro: queue de processos esperando
} Mutex;

void mutex_init(Mutex* m);
void mutex_lock(Mutex* m);
void mutex_unlock(Mutex* m);
int mutex_trylock(Mutex* m);

typedef struct {
    volatile uint32_t count;
} Semaphore;

void semaphore_init(Semaphore* s, uint32_t initial);
void semaphore_wait(Semaphore* s);
void semaphore_signal(Semaphore* s);
```

**Implementação:**

```c
// main/management/synchronization.c

void mutex_init(Mutex* m) {
    m->locked = 0;
}

void mutex_lock(Mutex* m) {
    while (1) {
        // Atomic compare-and-swap
        uint32_t expected = 0;
        uint32_t desired = 1;
        
        asm volatile(
            "lock cmpxchgl %2, %0\n"
            : "+m"(m->locked), "+a"(expected)
            : "r"(desired)
            : "cc"
        );
        
        if (expected == 0) {
            // Conseguiu o lock
            break;
        }
        
        // Espera (busy wait por enquanto)
        asm volatile("pause");
    }
}

void mutex_unlock(Mutex* m) {
    m->locked = 0;
}

int mutex_trylock(Mutex* m) {
    uint32_t expected = 0;
    uint32_t desired = 1;
    
    asm volatile(
        "lock cmpxchgl %2, %0\n"
        : "+m"(m->locked), "+a"(expected)
        : "r"(desired)
        : "cc"
    );
    
    return expected == 0 ? 1 : 0;  // 1 = sucesso, 0 = falho
}
```

---

### Complex Features (Complexo, Muito Impacto)

#### 8. Implementar Paging & Virtual Memory
**Dificuldade:** ⭐⭐⭐⭐⭐ (Muito Complexo)  
**Impacto:** CRÍTICO (escalabilidade)  
**Tempo Estimado:** 20-30 horas  

**Este é um projeto major - requer:**
- Page table management
- Paging structures (PML4, PDPT, PDT, PT)
- TLB flushing
- Page allocation/deallocation
- COW (Copy-on-Write) para fork
- Lazy allocation
- Segmentation fault detection

**Sugestão:** Dividir em sub-tasks:
1. Página table setup (4 horas)
2. Virtual memory mapping (6 horas)
3. Page fault handling (8 horas)
4. COW for fork (6 horas)
5. Testing (6 horas)

---

#### 9. Implementar Sistema de Arquivos FAT32
**Dificuldade:** ⭐⭐⭐⭐ (Muito Complexo)  
**Impacto:** CRÍTICO (persistência)  
**Tempo Estimado:** 25-35 horas  

**Sub-tasks:**
1. Boot sector parsing (4 horas)
2. FAT table reading (4 horas)
3. Directory listing (6 horas)
4. File reading (6 horas)
5. File writing (6 horas)
6. Fragmentation handling (4 horas)
7. LFN support (4 horas)
8. Testing (6 horas)

---

#### 10. Parser de Shell & Comandos
**Dificuldade:** ⭐⭐⭐⭐ (Muito Complexo)  
**Impacto:** Alto (usabilidade)  
**Tempo Estimado:** 15-20 horas  

**Sub-tasks:**
1. Lexer (4 horas)
2. Parser (6 horas)
3. AST execution (4 horas)
4. Pipes (3 horas)
5. Redirection (3 horas)
6. Testing (4 horas)

---

## 📚 Recursos Recomendados

### Documentação a Criar

```
docs/
├─ ARCHITECTURE.md - Overview da arquitetura
├─ MEMORY_LAYOUT.md - Esquema de memória
├─ BOOT_SEQUENCE.md - Boot process detalhado
├─ SYSCALL_ABI.md - System call interface
├─ DEVICE_DRIVERS.md - Driver architecture
├─ BUILD_INSTRUCTIONS.md - Como fazer build
├─ TESTING_GUIDE.md - Testing framework
├─ PERFORMANCE.md - Performance tuning
└─ DEBUGGING.md - Debugging techniques
```

### Ferramentas Úteis

- **Debuggers:** GDB + QEMU GDB stub
- **Disassemblers:** objdump, ndisasm
- **Profilers:** Simples timer-based
- **Memory tools:** Valgrind (futuro)
- **Linters:** splint, cppcheck

### Referências & Inspirações

- Linux kernel (https://kernel.org/)
- OSDev.org tutorials (https://wiki.osdev.org/)
- Minix OS (https://www.minix3.org/)
- XV6 MIT (https://github.com/mit-pdos/xv6-public)
- UEFI Specification
- Intel x86-64 ISA manual

---

## 🎯 Próximos Passos Recomendados

### Imediatos (Próximos 2 dias)

1. **Limpar código** - Remover imports comentados, dead code
2. **Documentar** - Criar `ARCHITECTURE.md` explicando status atual
3. **Implementar math.h** - Quick win, alta utilidade
4. **Adicionar logging** - Para debuggable kernel

### Curto Prazo (Próximas 2 semanas)

1. **Heap manager** - Resolve problema crítico de memória
2. **Exception handlers** - Impede crashes silenciosos
3. **Mutex/Semaphore** - Resolve race conditions
4. **Mais drivers** - Serial port, melhor teclado

### Médio Prazo (Próximos 2 meses)

1. **Paging & Virtual Memory** - Essencial para escalabilidade
2. **Sistema de Arquivos (FAT32)** - Essencial para persistência
3. **POSIX syscalls** - Interface padrão
4. **Mais built-in commands** - Melhor shell

### Longo Prazo (3-6 meses)

1. **Compiler toolchain** - GCC port
2. **Modo gráfico** - Restaurar graphics.h
3. **Rede** - Network driver & stack
4. **Production release** - vR2

---

## 📊 Métricas de Progresso

Sugestão para rastrear progresso:

```
Versão | Heap | Paging | FS | Syscalls | Shell | Estabilidade
-------|------|--------|----|---------|----|---------------
0.1    | ❌   | ❌     | ❌ | 0       | 20%| 40%
0.2    | ✅   | ❌     | ❌ | 0       | 20%| 50%
0.3    | ✅   | ✅     | ❌ | 0       | 30%| 70%
1.0    | ✅   | ✅     | ✅ | 30      | 60%| 80%
2.0    | ✅   | ✅     | ✅ | 50      | 80%| 85%
3.0    | ✅   | ✅     | ✅ | 80      | 95%| 90%
Release| ✅   | ✅     | ✅ | 100     | 100%| 95%
```

---

**Última Atualização:** 2026-06-07  
**Mantido por:** TeamImagine  
**Status:** Em Desenvolvimento Ativo

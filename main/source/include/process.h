#ifndef PROCESS_H
#define PROCESS_H

#include <stdint.h>
#include <stdbool.h>

// Estados do processo
typedef enum {
    PROCESS_READY,
    PROCESS_RUNNING,
    PROCESS_BLOCKED,
    PROCESS_TERMINATED
} ProcessState;

// Contexto de CPU (registros salvos durante context switch)
typedef struct {
    uint64_t rax, rbx, rcx, rdx;
    uint64_t rsi, rdi, rbp, rsp;
    uint64_t r8, r9, r10, r11, r12, r13, r14, r15;
    uint64_t rip;  // Instruction pointer
    uint64_t rflags;
    uint64_t cr3;  // Page table (para futuro)
} CpuContext;

// PCB (Process Control Block)
typedef struct Process {
    uint32_t pid;           // ID único do processo
    ProcessState state;     // Estado atual
    CpuContext context;     // Contexto de CPU
    uint64_t stack_top;     // Topo da pilha
    uint64_t stack_base;    // Base da pilha
    struct Process* next;   // Próximo na lista (para scheduler)
} Process;

// Variáveis globais
extern Process* process_list;
extern Process* current_process;

// Funções básicas
void process_init();
Process* process_create(void (*entry_point)());
void process_switch(Process* old, Process* new);
void process_yield();

#endif
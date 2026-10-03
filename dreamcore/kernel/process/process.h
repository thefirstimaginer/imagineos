#ifndef PROCESS_H
#define PROCESS_H

#include <stdint.h>
#include <stdbool.h>

// Estados do processo
typedef struct {
    uint64_t rax, rbx, rcx, rdx;
    uint64_t rsi, rdi, rbp;
    uint64_t r8, r9, r10, r11, r12, r13, r14, r15;
    uint64_t rip, rsp, rflags;
} UserFrame;
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
    bool in_use;
    const char* name;       // Nome do programa
    ProcessState state;     // Estado atual
    CpuContext context;     // Contexto de CPU
    UserFrame user_frame;   // Estado de retorno para userspace
    uint32_t parent_pid;
    int exit_status;
    uint32_t waiting_for_pid;
    uint64_t waiting_status;
    uint64_t stack_top;     // Topo da pilha
    uint64_t stack_base;    // Base da pilha
    struct Process* next;   // Próximo na lista (para scheduler)
} Process;

// Variáveis globais
extern Process* process_list;
extern Process* current_process;
extern uint32_t process_count;

// Funções básicas
void process_init();
Process* process_create(void (*entry_point)());
Process* process_create_named(const char* name, void (*entry_point)());
Process* process_create_user(const char* name, uint64_t cr3);
const char* process_state_name(ProcessState state);
void process_switch(Process* old, Process* new);
void process_yield();
Process* process_find(uint32_t pid);
void process_mark_exit(Process* process, int status);
void process_reap(Process* process);
void process_capture_user_frame(void);

#endif
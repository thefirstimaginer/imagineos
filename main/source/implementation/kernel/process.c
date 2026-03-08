#include "process.h"
#include "x86_64/port.h"
#include <stdint.h>
#include <stddef.h>

// Lista de processos (simples linked list)
static Process* process_list = NULL;
static Process* current_process = NULL;
static uint32_t next_pid = 1;

// Pilha por processo (4KB por enquanto, sem alocação dinâmica)
#define PROCESS_STACK_SIZE 4096

// Função dummy para idle process
static void idle_process() {
    while (1) {
        // Nada, apenas idle
    }
}

// Inicializa o sistema de processos
void process_init() {
    // Cria processo idle
    Process* idle = process_create(idle_process);
    if (idle) {
        idle->pid = 0;  // PID especial para idle
        current_process = idle;
    }
}

// Cria um novo processo
Process* process_create(void (*entry_point)()) {
    Process* proc = (Process*) 0x100000;  // Alocação dummy (fixo por enquanto)
    if (!proc) return NULL;

    proc->pid = next_pid++;
    proc->state = PROCESS_READY;
    proc->stack_base = (uint64_t) proc + sizeof(Process);
    proc->stack_top = proc->stack_base + PROCESS_STACK_SIZE;

    // Inicializa contexto
    proc->context.rax = 0;
    proc->context.rbx = 0;
    proc->context.rcx = 0;
    proc->context.rdx = 0;
    proc->context.rsi = 0;
    proc->context.rdi = 0;
    proc->context.rbp = proc->stack_top;
    proc->context.rsp = proc->stack_top;
    proc->context.r8 = 0;
    proc->context.r9 = 0;
    proc->context.r10 = 0;
    proc->context.r11 = 0;
    proc->context.r12 = 0;
    proc->context.r13 = 0;
    proc->context.r14 = 0;
    proc->context.r15 = 0;
    proc->context.rip = (uint64_t) entry_point;
    proc->context.rflags = 0x202;  // Interrupts enabled
    proc->context.cr3 = 0;  // Sem paging ainda

    // Adiciona à lista
    proc->next = process_list;
    process_list = proc;

    return proc;
}

// Context switch (assembly inline por enquanto)
void process_switch(Process* old, Process* new) {
    if (old == new) return;

    // Salva contexto antigo
    asm volatile (
        "pushfq\n"
        "push %%rax\n"
        "push %%rbx\n"
        "push %%rcx\n"
        "push %%rdx\n"
        "push %%rsi\n"
        "push %%rdi\n"
        "push %%rbp\n"
        "push %%r8\n"
        "push %%r9\n"
        "push %%r10\n"
        "push %%r11\n"
        "push %%r12\n"
        "push %%r13\n"
        "push %%r14\n"
        "push %%r15\n"
        : "=m" (old->context)
    );

    // Restaura contexto novo
    asm volatile (
        "pop %%r15\n"
        "pop %%r14\n"
        "pop %%r13\n"
        "pop %%r12\n"
        "pop %%r11\n"
        "pop %%r10\n"
        "pop %%r9\n"
        "pop %%r8\n"
        "pop %%rbp\n"
        "pop %%rdi\n"
        "pop %%rsi\n"
        "pop %%rdx\n"
        "pop %%rcx\n"
        "pop %%rbx\n"
        "pop %%rax\n"
        "popfq\n"
        "ret\n"  // Volta para RIP salvo
        : : "m" (new->context)
    );
}

// Yield (cede controle)
void process_yield() {
    // Simples: alterna para próximo processo
    if (!process_list) return;

    Process* next = current_process->next ? current_process->next : process_list;
    if (next != current_process) {
        Process* old = current_process;
        current_process = next;
        process_switch(old, current_process);
    }
}
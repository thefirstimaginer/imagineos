#include "process.h"
#include "port.h"
#include <stdint.h>
#include <stddef.h>

// Lista de processos (simples linked list)
Process* process_list = NULL;
Process* current_process = NULL;
uint32_t process_count = 0;
static uint32_t next_pid = 1;

// Pilha por processo (4KB por enquanto, sem alocação dinâmica)
#define PROCESS_STACK_SIZE 4096
#define MAX_PROCESSES 32

static Process process_pool[MAX_PROCESSES];
static uint8_t process_stacks[MAX_PROCESSES][PROCESS_STACK_SIZE];
extern UserFrame syscall_user_frame;

// Função dummy para idle process
static void idle_process() {
    while (1) {
        // Nada, apenas idle
    }
}

// Inicializa o sistema de processos
void process_init() {
    // Cria processo idle
    Process* idle = process_create_named("idle", idle_process);
    if (idle) {
        idle->pid = 0;  // PID especial para idle
        current_process = idle;
    }
}

// Cria um novo processo
Process* process_create(void (*entry_point)()) {
    return process_create_named("process", entry_point);
}

Process* process_create_user(const char* name, uint64_t cr3) {
    Process* process = process_create_named(name, idle_process);
    if (process != NULL) {
        process->context.cr3 = cr3;
        process->state = PROCESS_RUNNING;
        if (current_process != NULL && current_process->pid == 0) {
            current_process->state = PROCESS_BLOCKED;
            current_process = process;
        }
    }
    return process;
}

Process* process_create_named(const char* name, void (*entry_point)()) {
    if (process_count >= MAX_PROCESSES || !entry_point) return NULL;

    Process* proc = &process_pool[process_count];

    proc->pid = next_pid++;
    proc->name = name;
    proc->state = PROCESS_READY;
    proc->parent_pid = current_process != NULL ? current_process->pid : 0;
    proc->exit_status = 0;
    proc->waiting_for_pid = 0;
    proc->stack_base = (uint64_t) process_stacks[process_count];
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
    process_count++;

    return proc;
}

Process* process_find(uint32_t pid) {
    Process* process = process_list;

    while (process != NULL) {
        if (process->pid == pid) return process;
        process = process->next;
    }
    return NULL;
}

void process_mark_exit(Process* process, int status) {
    if (process == NULL) return;
    process->exit_status = status;
    process->state = PROCESS_TERMINATED;
}

void process_reap(Process* process) {
    if (process == NULL) return;
    process->state = PROCESS_BLOCKED;
    process->parent_pid = 0;
    process->exit_status = 0;
    process->waiting_for_pid = 0;
}

void process_capture_user_frame(void) {
    if (current_process != NULL) {
        current_process->user_frame = syscall_user_frame;
    }
}

const char* process_state_name(ProcessState state) {
    switch (state) {
        case PROCESS_READY: return "READY";
        case PROCESS_RUNNING: return "RUNNING";
        case PROCESS_BLOCKED: return "BLOCKED";
        case PROCESS_TERMINATED: return "TERMINATED";
        default: return "UNKNOWN";
    }
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
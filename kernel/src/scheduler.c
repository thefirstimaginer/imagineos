#include "scheduler.h"
#include "process.h"
#include "pic.h"
#include "port.h"
#include "paging.h"
#include <stddef.h>

// Contador de ticks
static uint32_t tick_count = 0;

// Inicializa scheduler
void scheduler_init() {
    // Configura PIT para ~100Hz (timer interrupt)
    // PIT channel 0, mode 3 (square wave), divisor para ~100Hz
    uint16_t divisor = 1193;  // 1193180 / 100 ≈ 1193
    port_outb(0x43, 0x36);    // Command byte
    port_outb(0x40, divisor & 0xFF);
    port_outb(0x40, (divisor >> 8) & 0xFF);
}

// Handler do timer interrupt
void scheduler_tick() {
    tick_count++;
    pic_eoi_master();  // Acknowledge interrupt

    // A cada 10 ticks, agenda (para ~10Hz scheduling)
    if (tick_count % 10 == 0) {
        scheduler_schedule();
    }
}

static void save_user_frame(Process *process, InterruptFrame *frame) {
    process->user_frame.rax = frame->rax;
    process->user_frame.rbx = frame->rbx;
    process->user_frame.rcx = frame->rcx;
    process->user_frame.rdx = frame->rdx;
    process->user_frame.rsi = frame->rsi;
    process->user_frame.rdi = frame->rdi;
    process->user_frame.rbp = frame->rbp;
    process->user_frame.r8 = frame->r8;
    process->user_frame.r9 = frame->r9;
    process->user_frame.r10 = frame->r10;
    process->user_frame.r11 = frame->r11;
    process->user_frame.r12 = frame->r12;
    process->user_frame.r13 = frame->r13;
    process->user_frame.r14 = frame->r14;
    process->user_frame.r15 = frame->r15;
    process->user_frame.rip = frame->rip;
    process->user_frame.rsp = frame->rsp;
    process->user_frame.rflags = frame->rflags;
}

static void load_user_frame(Process *process, InterruptFrame *frame) {
    frame->rax = process->user_frame.rax;
    frame->rbx = process->user_frame.rbx;
    frame->rcx = process->user_frame.rcx;
    frame->rdx = process->user_frame.rdx;
    frame->rsi = process->user_frame.rsi;
    frame->rdi = process->user_frame.rdi;
    frame->rbp = process->user_frame.rbp;
    frame->r8 = process->user_frame.r8;
    frame->r9 = process->user_frame.r9;
    frame->r10 = process->user_frame.r10;
    frame->r11 = process->user_frame.r11;
    frame->r12 = process->user_frame.r12;
    frame->r13 = process->user_frame.r13;
    frame->r14 = process->user_frame.r14;
    frame->r15 = process->user_frame.r15;
    frame->rip = process->user_frame.rip;
    frame->rsp = process->user_frame.rsp;
    frame->rflags = process->user_frame.rflags;
}

void scheduler_user_tick(InterruptFrame *frame) {
    Process *next;

    tick_count++;
    pic_eoi_master();
    if ((frame->cs & 3) != 3 || current_process == NULL) return;
    save_user_frame(current_process, frame);
    if (tick_count % 10 != 0) return;

    next = current_process->next != NULL ? current_process->next : process_list;
    while (next != current_process && next->state != PROCESS_READY) {
        next = next->next != NULL ? next->next : process_list;
    }
    if (next == current_process || next->state != PROCESS_READY) return;

    current_process->state = PROCESS_READY;
    next->state = PROCESS_RUNNING;
    current_process = next;
    paging_activate(next->context.cr3);
    load_user_frame(next, frame);
}

uint32_t scheduler_ticks(void) {
    return tick_count;
}

// Agenda próximo processo ready
void scheduler_schedule() {
    // Simples round-robin
    extern Process* process_list;
    extern Process* current_process;

    if (!process_list) return;

    Process* next = current_process->next ? current_process->next : process_list;
    while (next != current_process && next->state != PROCESS_READY) {
        next = next->next ? next->next : process_list;
    }

    if (next != current_process && next->state == PROCESS_READY) {
        Process* old = current_process;
        current_process = next;
        current_process->state = PROCESS_RUNNING;
        old->state = PROCESS_READY;
        process_switch(old, current_process);
    }
}
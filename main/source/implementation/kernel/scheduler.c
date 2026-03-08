#include "scheduler.h"
#include "process.h"
#include "x86_64/pic.h"

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
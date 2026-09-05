#ifndef SCHEDULER_H
#define SCHEDULER_H

#include "process.h"

typedef struct {
	uint64_t r15, r14, r13, r12, r11, r10, r9, r8;
	uint64_t rdi, rsi, rbp, rdx, rcx, rbx, rax;
	uint64_t rip, cs, rflags, rsp, ss;
} InterruptFrame;

// Inicializa scheduler
void scheduler_init();

// Handler do timer (chamado por interrupt)
void scheduler_tick();
uint32_t scheduler_ticks(void);
void scheduler_user_tick(InterruptFrame *frame);

// Agenda próximo processo
void scheduler_schedule();

#endif
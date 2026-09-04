#ifndef SCHEDULER_H
#define SCHEDULER_H

#include "process.h"

// Inicializa scheduler
void scheduler_init();

// Handler do timer (chamado por interrupt)
void scheduler_tick();

// Agenda próximo processo
void scheduler_schedule();

#endif
#pragma once

#include <stdint.h>

void idt_init();
void idt_set_handler_keyboard(void (*handler)());
void idt_handle_exception(uint64_t vector, uint64_t *stack) __attribute__((noreturn));

#include "kassert.h"
#include "kprintf.h"

void kassert_fail(const char *expression, const char *file, int line) {
    kputs("kernel assertion failed: ");
    kputs(expression);
    kputs(" at ");
    kputs(file);
    kputchar(':');
    (void)line;
    for (;;) __asm__ volatile ("hlt");
}
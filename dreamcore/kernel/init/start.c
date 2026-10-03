#include <limine.h>

extern void kernel_main(void);

void _start(void)
{
    kernel_main();

    for (;;) {
        __asm__ volatile ("cli");
        __asm__ volatile ("hlt");
    }
}
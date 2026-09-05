#include "kprintf.h"
#include "print.h"

void kputchar(char character) {
    print_char(character);
}

void kputs(const char *string) {
    print_str((char *)string);
}
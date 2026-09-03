#include "print.h"
#include "string.h"

void clear_init(void) {
    return;
}

void clear_run(char* args) {
    (void)args;
    print_clear();
    shell_print_prompt();
}
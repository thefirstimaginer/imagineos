#include "print.h"
#include "modules.h"

void liteinterp_init(void) {
    print_str("[lite] interpreter ready\n");
}

void liteinterp(char* args) {
    (void)args;
    print_str("[lite] placeholder interpreter running\n");
}

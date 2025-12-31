#include "print.h"
#include "shell.h"
#include "keyboard.h"
#include "x86_64/rtc.h"
#include "bool.h"
#include "keyboard_keys.h"


void keyboard_keys_init();           // inicializa as teclas

void kernel_main() { // É onde o sistema roda

    print_clear();
    shell_init();

    keyboard_init();
    keyboard_set_handler(handle_input);
    
    while (1);
}

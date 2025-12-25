#include "print.h"
#include "keyboard.h"
#include "x86_64/rtc.h"
#include "framebuffer.h"
#include "bool.h"
#include "keyboard_keys.h"


void keyboard_keys_init();           // inicializa as teclas

void kernel_main(uint64_t mb_info) { // É onde o sistema roda

    // Initialize framebuffer if GRUB provided one
    framebuffer_init_from_multiboot(mb_info);
    print_clear();
    print_set_color(PRINT_COLOR_WHITE, PRINT_COLOR_BLACK);
    /*
    Essa coisa aqui serve como régua pra eu não me perder nas medidas
    print_str("12341234123412341234123412341234123412341234123412341234123412341234123412341234\n");
    */
    print_str("Imagine System Shell V1.0_DP - BUILD 20251225\n");
    print_str("\n");
    
    
    print_str("Press [ENTER] to start using the shell.\n");
    print_str("Type 'help' for help.\n");

    keyboard_init();
    keyboard_set_handler(handle_input);
    
    while (1);
}

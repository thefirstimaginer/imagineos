#include "print.h"
#include "shell.h"
#include "keyboard.h"
#include "userspace/login.h"
#include "rtc.h"
// #include "graphics.h"  // Driver gráfico - arquivo removido
#include "bool.h"
#include "keyboard_keys.h"
#include "init.h"
#include "scheduler.h"
//#include "main.h"

void kernel_main()                   // É onde o sistema roda
{
    init_system();
    scheduler_init();  // Inicializa scheduler
    
    keyboard_keys_init();   // inicializa as teclas
    keyboard_init();
    keyboard_set_handler(handle_input);
    
    // Loop principal: permite scheduling
    while (1) {
        // Idle loop - scheduler é chamado por timer interrupt
        asm volatile("hlt");  // Halt até próxima interrupt
    }
}

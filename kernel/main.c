#include "print.h"
#include "userspace/quackshell/shell.h"
#include "keyboard_keys.h"
#include "keyboard.h"
#include "userspace/system/login.h"
#include "x86_64/rtc.h"
// #include "graphics.h"  // Driver gráfico - arquivo removido
#include "libraries/bool.h"
#include "management/init.h"
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

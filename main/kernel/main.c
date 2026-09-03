#include "print.h"
#include "shell.h"
#include "keyboard.h"
#include "userspace/login.h"
#include "x86_64/rtc.h"
// #include "graphics.h"  // Driver gráfico - arquivo removido
#include "bool.h"
#include "keyboard_keys.h"
#include "modules.h"
#include "process.h"
#include "scheduler.h"
//#include "main.h"

void kernel_main()                   // É onde o sistema roda
{
    process_init();  // Inicializa sistema de processos
    scheduler_init();  // Inicializa scheduler

    modules_load(); // Carrega os módulos antes do shell
    
    // Inicia o prompt de login primeiro
    login_prompt();
    
    keyboard_keys_init();   // inicializa as teclas
    keyboard_init();
    keyboard_set_handler(handle_input);
    
    // Loop principal: permite scheduling
    while (1) {
        // Idle loop - scheduler é chamado por timer interrupt
        asm volatile("hlt");  // Halt até próxima interrupt
    }
}

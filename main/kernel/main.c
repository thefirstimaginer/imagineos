#include "print.h"
#include "shellMain.h"
#include "keyboard.h"
#include "x86_64/rtc.h"
// #include "graphics.h"  // Driver gráfico - arquivo removido
#include "bool.h"
#include "keyboard_keys.h"
#include "modules.h"
#include "process.h"
#include "scheduler.h"
//#include "main.h"


void keyboard_keys_init();           // inicializa as teclas

void kernel_main()                   // É onde o sistema roda
{
    // graphics_init();  // Temporariamente removido para testar se o kernel inicia sem modo gráfico
    // graphics_clear(COLOR_BLACK);  // Limpa a tela gráfica

    // Opcional: Desenha um texto inicial gráfico
    // graphics_draw_string(10, 10, "ImagineOS Graphics Mode Initialized", COLOR_WHITE, COLOR_BLACK);

    process_init();  // Inicializa sistema de processos
    scheduler_init();  // Inicializa scheduler

    modules_load(); // Carrega os módulos antes do shell
    shell_init();

    keyboard_init();
    keyboard_set_handler(handle_input);
    
    // Loop principal: permite scheduling
    while (1) {
        // Idle loop - scheduler é chamado por timer interrupt
        asm volatile("hlt");  // Halt até próxima interrupt
    }
}

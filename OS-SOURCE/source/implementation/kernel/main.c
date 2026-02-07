#include "print.h"
#include "shell.h"
#include "keyboard.h"
#include "x86_64/rtc.h"
#include "bool.h"
#include "keyboard_keys.h"

#include "modules.h"


void keyboard_keys_init();           // inicializa as teclas

void kernel_main()                   // É onde o sistema roda
{
    /*
        #TODO
        Criar uma função "modules_load()" que carrega 
        todos os módulos antes do shell.

        #TODO
        Criar uma variável para quando os módulos são
        carregados, eles indicam que podem ser
        executados pelo sistema.
    */
    print_clear();
    modules_load(); // Carrega os módulos antes do shell
    shell_init();

    keyboard_init();
    keyboard_set_handler(handle_input);
    
    while (1);
}

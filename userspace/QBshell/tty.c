/* Imagine Operating System - vR1_0.1.0 - Codename "Jessica"
 * Copyright (c) 2026, Adryan Alcantara & TeamImagine
 * Licensed Under IGPLv1
 *
 * Shell Interpreter for ImagineOS.
 */

#include "print.h" /*
                    * Será removido no futuro para dar lugar a um
                    * driver de vídeo com suporte a gráficos e texto
                    */

#include "shell.h" // Será desenvolvido em breve
#include "tty.h"
#include "userspace/login.h"

#include "x86_64/rtc.h"
//#include "graphics.h"

/* precisamos de memset, que está declarado em libimagine.h */
#include "libraries/libimagine.h"   // inclui string/math e protótipos de memória
// NOTA: A LIBIMAGINE será removida no futuro...

#include "modules.h"

char last_command[128] = {0};
static char input_buffer[256] = {0};
static int input_index = 0;

static int shell_x = 10;
static int shell_y = 170;

void shell_print_prompt(void) {
    print_set_color(PRINT_COLOR_GREEN, PRINT_COLOR_BLACK);
    print_str((char*)login_get_username());
    print_str("@");
    print_str((char*)login_get_hostname());
    print_str(":~$ ");
    print_set_color(PRINT_COLOR_WHITE, PRINT_COLOR_BLACK);
}

void shell_init() {
    print_clear();
    
    print_str("Welcome to Imagine Operating System!\n");
    print_str("JESSICA R1 - Under Construction\n");
    
    shell_print_prompt();
    input_index = 0;
}

void shell_handle_enter(void) {
    char* cmd = input_buffer;
    
    char cmd_name[32] = {0};
    char* args = "";
    
    char* space = strchr(cmd, ' ');
    if (space) {
        strncpy(cmd_name, cmd, space - cmd);
        args = space + 1;
    } else {
        strncpy(cmd_name, cmd, 31);
    }
    
    if (cmd_name[0] == '\0') {
        print_str("\n");
        shell_print_prompt();
        input_index = 0;
        return;
    }

    strncpy(last_command, cmd, 127);
    
    // Buscar e executar o módulo correspondente ao comando
    int found = 0;
    for (int i = 0; i < modules_count; i++) {
        if (strcmp(modules[i].name, cmd_name) == 0) {
            // Encontrou o módulo, executar a função run
            if (modules[i].run) {
                modules[i].run(args);
            }
            found = 1;
            break;
        }
    }
    
    if (!found) {
        print_str("Comando não encontrado: ");
        print_str(cmd_name);
        print_str("\n");
    }

    print_str("\n");
    shell_print_prompt();
    input_index = 0;
    memset(input_buffer, 0, 256);
}

void shell_add_char(char c) {
    if (c == '\b') {
        if (input_index > 0) {
            input_index--;
            input_buffer[input_index] = '\0';
            // Usar a função backspace de print.c que gerencia o cursor
            backspace();
        }
        return;
    } else if (c == '\n') {
        input_buffer[input_index] = '\0';
        shell_handle_enter();
        return;
    } else if (c != '\r' && input_index < 255) {
        input_buffer[input_index] = c;
        input_index++;
        input_buffer[input_index] = '\0';
        print_char(c);
    }
}

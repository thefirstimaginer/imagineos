#include "print.h"
#include "keyboard.h"
#include "x86_64/rtc.h"
#include "comandos.h"

char to_ascii(uint16_t code, bool shift_active){
    char base_char;
    switch (code) {
        case 
            KEY_CODE_A: base_char = 'A';
        break;
        case
            KEY_CODE_B: base_char = 'B';
        break;
        case 
            KEY_CODE_C: base_char = 'C'; 
        break;
        case 
            KEY_CODE_D: base_char = 'D'; 
        break;
        case 
            KEY_CODE_E: base_char = 'E'; 
        break;
        case 
            KEY_CODE_F: base_char = 'F'; 
        break;
        case 
            KEY_CODE_G: base_char = 'G'; 
        break;
        case 
            KEY_CODE_H: base_char = 'H';
        break;
        case 
            KEY_CODE_I: base_char = 'I'; 
        break;
        case 
            KEY_CODE_J: base_char = 'J'; 
        break;
        case
            KEY_CODE_K: base_char = 'K'; 
        break;
        case 
            KEY_CODE_L: base_char = 'L'; 
        break;
        case 
            KEY_CODE_M: base_char = 'M'; 
        break;
        case 
            KEY_CODE_N: base_char = 'N'; 
        break;
        case 
            KEY_CODE_O: base_char = 'O'; 
        break;
        case 
            KEY_CODE_P: base_char = 'P'; 
        break;
        case 
            KEY_CODE_Q: base_char = 'Q'; 
        break;
        case 
            KEY_CODE_R: base_char = 'R'; 
        break;
        case 
            KEY_CODE_S: base_char = 'S'; 
        break;
        case 
            KEY_CODE_T: base_char = 'T'; 
        break;
        case 
            KEY_CODE_U: base_char = 'U'; 
        break;
        case 
            KEY_CODE_V: base_char = 'V'; 
        break;
        case 
            KEY_CODE_W: base_char = 'W'; 
        break;
        case 
            KEY_CODE_X: base_char = 'X'; 
        break;
        case 
            KEY_CODE_Y: base_char = 'Y'; 
        break;
        case 
            KEY_CODE_Z: base_char = 'Z'; 
        break;
        // Para Números
        case 
            KEY_CODE_1: base_char = '1'; 
        break;
        case 
            KEY_CODE_2: base_char = '2'; 
        break;
        case 
            KEY_CODE_3: base_char = '3'; 
        break;
        case 
            KEY_CODE_4: base_char = '4'; 
        break;
        case 
            KEY_CODE_5: base_char = '5'; 
        break;
        case 
            KEY_CODE_6: base_char = '6'; 
        break;
        case 
            KEY_CODE_7: base_char = '7'; 
        break;
        case 
            KEY_CODE_8: base_char = '8'; 
        break;
        case
            KEY_CODE_9: base_char = '9'; 
        break;
        case 
            KEY_CODE_0: base_char = '0'; 
        break;
        // Teclas de Confirmação
        case 
            KEY_CODE_SPACE: base_char = ' '; 
        break;
        case 
            KEY_CODE_ENTER: base_char = '\n'; 
        break;
        case 
            KEY_CODE_BACKSPACE: base_char = '\b'; 
        break;
        // Símbolos
        case KEY_CODE_MINUS: base_char = '-'; break;
        case KEY_CODE_EQUALS: base_char = '='; break;
        case KEY_CODE_LBRACKET: base_char = '['; break;
        case KEY_CODE_RBRACKET: base_char = ']'; break;
        case KEY_CODE_SEMICOLON: base_char = ';'; break;
        case KEY_CODE_APOSTROPHE: base_char = '\''; break;
        case KEY_CODE_GRAVE: base_char = '`'; break;
        case KEY_CODE_BACKSLASH: base_char = '\\'; break;
        case KEY_CODE_COMMA: base_char = ','; break;
        case KEY_CODE_PERIOD: base_char = '.'; break;
        case KEY_CODE_SLASH: base_char = '/'; break;

        default:
            return '?';
    }

    // Se o Shift estiver pressionado, convertemos o caractere base
    if (shift_active) {
        switch (base_char) {
            // Números -> Símbolos
            case '1': 
                return '!';
            case '2': 
                return '@';
            case '3': 
                return '#';
            case '4': 
                return '$';
            case '5': 
                return '%';
            case '6': 
                return '^'; // Em alguns layouts pode ser '¨'
            case '7': 
                return '&';
            case '8': 
                return '*';
            case '9': 
                return '(';
            case '0': 
                return ')';
            
            // Símbolos -> Símbolos Shiftados
            case '`': return '~';
            case '-': return '_';
            case '=': return '+';
            case '[': return '{';
            case ']': return '}';
            case '\\': return '|';
            case ';': return ':';
            case '\'': return '"';
            case ',': return '<';
            case '.': return '>';
            case '/': return '?';
        }
    }

    if (base_char >= 'A' && base_char <= 'Z' && !shift_active) {
        // Converte para minúscula (ASCII 'A' + 32 = 'a')
        return base_char + 32;
    }

    return base_char;
}

void handle_input(struct KeyboardEvent event) {
    if (event.type == KEYBOARD_EVENT_TYPE_MAKE) {
        
        // Converte o código da tecla para um caractere
       char c = to_ascii(event.code,event.shift_active);
        
        // Decide o que fazer
        if (c == '\b') {
            // Se for backspace, chama a nova função
            backspace(); 
        } 
        else if (c == '\n') {
            // Tratar o ENTER (pular linha) também é bom
            print_set_color(PRINT_COLOR_WHITE, PRINT_COLOR_BLACK);
            print_char(c);
            print_str("shell:$ ");
            //get_str("%c", &comando); //Pega os comandos e armazena na variável "comando"
        }
        else if (c != '?') { // Ignora teclas não mapeadas
            // É um caractere normal (A, B, C...)
            print_set_color(PRINT_COLOR_WHITE, PRINT_COLOR_BLACK);
            print_char(c);
        }

    } else if (event.type == KEYBOARD_EVENT_TYPE_BREAK) {
        // ...
    }
}

void kernel_main() { // É onde o sistema roda
    
    //char comando;

    print_clear();
    print_set_color(PRINT_COLOR_WHITE, PRINT_COLOR_BLACK);
    /*
    Essa coisa aqui serve como régua pra eu não me perder nas medidas
    print_str("12341234123412341234123412341234123412341234123412341234123412341234123412341234\n");
    */
    print_str("+------------------------------------------------------------------------------+\n");
    print_str("|                                                                              |\n");
    print_str("|                       Imagine Operating System Shell                         |\n");
    print_str("|      Copyright (C) 2024-2025 Imagine Technologies, All Rights Reserved.      |\n");
    print_str("|                              Version 0.0.1                                   |\n");
    print_str("|                                                                              |\n");
    print_str("+------------------------------------------------------------------------------+\n");
    print_str("|                            DEVELOPMENT PREVIEW                               |\n");
    print_str("+------------------------------------------------------------------------------+\n");
    print_str(" \n");
    
    keyboard_init();
    keyboard_set_handler(handle_input);
    
    /*
    uint8_t prev_seconds = 0;
    
    
    for (uint8_t int_numbers = 0; int_numbers < 1;) { // uint unsigned integer, inteiro sem sinal
                                  // 8 armazena 8bits na memoria
                                  // _t indica que é um tipo de "largura exata" definida no
                                  // cabeçalho <stdint.h>, pra garantir o tamanho, independente do sistema

        uint8_t seconds = rtc_seconds();
        
        if (seconds != prev_seconds) {
            int_numbers = int_numbers + 1;
            // or int_numbers++
            
            print_set_color(PRINT_COLOR_WHITE, PRINT_COLOR_BLACK);
            print_str("%d - Seconds: \n", );
            print_uint64_dec(seconds);
            
        }
        
        prev_seconds = seconds;
    }
    */
    print_str("READY! - Press [ENTER]");
    print_str(" \n");
    
    while (1);
}

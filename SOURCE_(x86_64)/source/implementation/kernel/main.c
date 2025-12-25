#include "print.h"
#include "keyboard.h"
#include "x86_64/rtc.h"
#include "framebuffer.h"
#include "bool.h"

char to_ascii(uint16_t code, bool shift_active, bool caps_active, bool num_lock_active){
    char base_char;
    // Handle numpad when Num Lock is active. Map PS/2 scancodes for keypad
    // (set 1): 0x47..0x53 are 7,8,9,4,5,6,1,2,3,0 and 0x52=0,0x53=., 0x4A=-,0x4E=+
    // Also accept extended (0xE0<<8 | scancode) forms produced by keyboard.c.
    switch (code) {
        case 0x47: case (0xE000 | 0x47): return num_lock_active ? '7' : '?';
        case 0x48: case (0xE000 | 0x48): return num_lock_active ? '8' : '?';
        case 0x49: case (0xE000 | 0x49): return num_lock_active ? '9' : '?';
        case 0x4B: case (0xE000 | 0x4B): return num_lock_active ? '4' : '?';
        case 0x4C: case (0xE000 | 0x4C): return num_lock_active ? '5' : '?';
        case 0x4D: case (0xE000 | 0x4D): return num_lock_active ? '6' : '?';
        case 0x4F: case (0xE000 | 0x4F): return num_lock_active ? '1' : '?';
        case 0x50: case (0xE000 | 0x50): return num_lock_active ? '2' : '?';
        case 0x51: case (0xE000 | 0x51): return num_lock_active ? '3' : '?';
        case 0x52: case (0xE000 | 0x52): return num_lock_active ? '0' : '?';
        case 0x53: case (0xE000 | 0x53): return num_lock_active ? '.' : '?';
        case 0x35: case KEY_CODE_NUMPAD_SLASH: return '/';
        case KEY_CODE_NUMPAD_ENTER: return '\n';
        case 0x37: case (0xE000 | 0x37): return num_lock_active ? '*' : '?';
        case 0x4E: case (0xE000 | 0x4E): return num_lock_active ? '+' : '?';
        case 0x4A: case (0xE000 | 0x4A): return num_lock_active ? '-' : '?';
    }

    
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
    // For letters, CapsLock toggles case: uppercase when (shift_active XOR caps_active) is true
    if (base_char >= 'A' && base_char <= 'Z') {
        bool make_upper = shift_active ^ caps_active;
        if (!make_upper) {
            return base_char + 32; // lowercase
        }
        return base_char; // uppercase
    }

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

    return base_char;
}

void handle_input(struct KeyboardEvent event) {
    if (event.type == KEYBOARD_EVENT_TYPE_MAKE) {
        
        // Converte o código da tecla para um caractere
    char c = to_ascii(event.code,event.shift_active, event.caps_active, event.num_lock_active);
        
        // Decide o que fazer
        if (c == '\b') {
            // Se for backspace, chama a nova função
            backspace(); 
        } 
        else if (c == '\n') {
            // Tratar o ENTER (pular linha) e processar comandos
            print_set_color(PRINT_COLOR_WHITE, PRINT_COLOR_BLACK);
            shell_handle_enter();
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

void kernel_main(uint64_t mb_info) { // É onde o sistema roda

    // Initialize framebuffer if GRUB provided one
    framebuffer_init_from_multiboot(mb_info);
    print_clear();
    print_set_color(PRINT_COLOR_WHITE, PRINT_COLOR_BLACK);
    /*
    Essa coisa aqui serve como régua pra eu não me perder nas medidas
    print_str("12341234123412341234123412341234123412341234123412341234123412341234123412341234\n");
    */
    print_str("Imagine System Shell V1.0_DP - BUILD 20251224\n");
    print_str("\n");
    
    
    print_str("Press [ENTER] to start using the shell.\n");
    keyboard_init();
    keyboard_set_handler(handle_input);
    
    while (1);
}

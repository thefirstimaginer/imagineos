//ILLUSION OS FILE
#include "print.h"
#include "x86_64/port.h"

static size_t NUM_COLS = 80;
static size_t NUM_ROWS = 25;

struct Char {
    uint8_t character;
    uint8_t color;
};

struct Char* buffer = (struct Char*) 0xb8000;
#include "framebuffer.h"

// framebuffer color used by renderer (RGB 32-bit)
static uint32_t fb_fg = 0x00FFFFFF;
static uint32_t fb_bg = 0x00000000;

// If framebuffer is available, compute rows/cols from its size (assume 8x16 glyph cell)
static void recompute_size_from_fb() {
    if (!fb_enabled) return;
    const size_t gw = 10;
    const size_t gh = 16;
    NUM_COLS = fb_width / gw;
    NUM_ROWS = fb_height / gh;
}
size_t col = 0;
size_t row = 0;
uint8_t color = PRINT_COLOR_WHITE | PRINT_COLOR_BLACK << 4;
// Shell prompt tracking: start column and row of the editable area after the prompt
static size_t shell_prompt_col = 0;
static size_t shell_prompt_row = 0;

void clear_row(size_t row) {
    struct Char empty = (struct Char) {
        character: ' ',
        color: color,
    };

    for (size_t col = 0; col < NUM_COLS; col++) {
        buffer[col + NUM_COLS * row] = empty;
    }
}

void print_clear() {
    if (fb_enabled) {
        recompute_size_from_fb();
        fb_clear(0x00000000);
        return;
    }
    for (size_t i = 0; i < NUM_ROWS; i++) {
        clear_row(i);
    }
}

void print_newline() {
    col = 0;

    if (fb_enabled) {
        fb_set_cursor_pos(col, row + 1);
        row++;
        return;
    }

    if (row < NUM_ROWS - 1) {
        row++;
        set_cursor(col, row);
        return;
    }

    for (size_t row = 1; row < NUM_ROWS; row++) {
        for (size_t col = 0; col < NUM_COLS; col++) {
            struct Char character = buffer[col + NUM_COLS * row];
            buffer[col + NUM_COLS * (row - 1)] = character;
        }
    }

    clear_row(NUM_ROWS - 1);
    set_cursor(col, row);
}

void print_char(char character) {
    if (character == '\n') {
        print_newline();
        return;
    }

    if (fb_enabled) {
        recompute_size_from_fb();
        fb_putc(character);
        return;
    }

    if (col > NUM_COLS) {
        print_newline();
    }

    buffer[col + NUM_COLS * row] = (struct Char) {
        character: (uint8_t) character,
        color: color,
    };

    col++;
    set_cursor(col, row);
}

void print_str(char* str) {
    for (size_t i = 0; 1; i++) {
        char character = (uint8_t) str[i];

        if (character == '\0') {
            return;
        }

        print_char(character);
    }
}

void print_set_color(uint8_t foreground, uint8_t background) {
    color = foreground + (background << 4);
    // map VGA 16-color palette to simple 32-bit RGB values for framebuffer
    static const uint32_t palette[16] = {
        0x00000000, // black
        0x000000AA, // blue
        0x0000AA00, // green
        0x0000AAAA, // cyan
        0x00AA0000, // red
        0x00AA00AA, // magenta
        0x00AA5500, // brown
        0x00AAAAAA, // light gray
        0x00555555, // dark gray
        0x005555FF, // light blue
        0x0055FF55, // light green
        0x0055FFFF, // light cyan
        0x00FF5555, // light red
        0x00FF55FF, // pink
        0x00FFFF55, // yellow
        0x00FFFFFF, // white
    };

    if (fb_enabled) {
        fb_fg = palette[foreground & 0x0F];
        fb_bg = palette[background & 0x0F];
        fb_set_colors(fb_fg, fb_bg);
    }
}

void print_uint64_dec(uint64_t value) {
    if (value == 0) {
        print_char('0');
        return;
    }
    
    char buffer[20];
    int i = 0;
    
    while (value > 0) {
        buffer[i++] = (value % 10) + '0';
        value /= 10;
    }
    
    while (i-- > 0) {
        print_char(buffer[i]);
    }
}

void print_uint64_hex(uint64_t value) {
    if (value == 0) {
        print_char('0');
        return;
    }
    
    char buffer[16];                       // temporary buffer
    int i = 0;                             // index for buffer
    
    while (value > 0) {
        uint8_t digit = value & 0xF;       // get last 4 bits
        
        if (digit < 10) {                  // 0-9
            buffer[i++] = digit + '0';     // convert to '0'-'9'
        } else {                           // A-F
            buffer[i++] = digit - 10 + 'A';// convert to 'A'-'F'
        }
        
        value >>= 4;// shift right by 4 bits
    }
    
    while (i-- > 0) {// print in reverse order
        print_char(buffer[i]);
    }
}

void print_uint64_bin(uint64_t value) { // print 64-bit value in binary
    char buffer[64];                    // temporary buffer
    
    for (size_t i = 0; i < 64; i++) {   // extract bits
        buffer[i] = (value & 1) + '0';  // get least significant bit
        value >>= 1;                    // shift right
    }
    
    for (size_t i = 64; i > 0; i--) {   // print in reverse order
        print_char(buffer[i - 1]);      //
    }
}

void backspace() {
    // 1. Verificar se o cursor está no início (coluna 0). 
    //    Se for 0, não fazemos nada (não voltamos para a linha anterior por simplicidade).
    // Não permitir apagar antes do início do prompt salvo
    if (row == shell_prompt_row && col <= shell_prompt_col) {
        return;
    }

    if (col > 0) {
        // 2. Mover o cursor para trás (lógica)
        col--;

        // 3. Sobrescrever o caractere anterior com um espaço em branco
        //    A cor do espaço é a cor de fundo atual, o que faz parecer que a letra "sumiu".
        buffer[col + NUM_COLS * row] = (struct Char) {
            character: ' ',
            color: color,
        };

        set_cursor(col, row);
        return;
    }

    // Se chegamos aqui, `col == 0`. Tentamos mover para a linha anterior.
    if (row == 0) {
        return;
    }

    // Move para a linha anterior na última coluna
    row--;
    col = NUM_COLS - 1;

    // Se ao retornar para a linha do prompt nós ultrapassarmos a área protegida,
    // colocamos o cursor no fim da área editável e não apagamos o prompt.
    if (row == shell_prompt_row && col < shell_prompt_col) {
        col = shell_prompt_col;
        return;
    }

    // Apaga o último caractere da linha anterior
    buffer[col + NUM_COLS * row] = (struct Char) {      // sobrescreve com espaço
        character: ' ',                                 // caractere espaço
        color: color,                                   // cor de fundo atual
    };
    set_cursor(col, row);                               // Atualiza o cursor
}

void shell_print_prompt() {                             // print shell prompt and set editable area
    // Imprime o prompt e só então registra o início da área editável
    print_str("shell:$ ");                              // imprime o prompt
    shell_prompt_col = col;                             // registra a coluna do prompt
    shell_prompt_row = row;                             // registra a linha do prompt
    enable_cursor(14, 15);                              // cursor block
    set_cursor(col, row);                               // garante que o cursor está na posição correta
}

void shell_handle_enter() {                             // process command entered at prompt
    // Read the typed command from the screen buffer (from prompt start to current cursor)
    size_t start = shell_prompt_row * NUM_COLS + 
    shell_prompt_col;                                   // start of editable area
    size_t end = row * NUM_COLS + col;                  // current cursor position
    size_t len = 0;                                     // length of command

    if (end > start) len = end - start;                 // compute length
    if (len > 127) len = 127;                           // limit length to buffer size

    char cmd[128];                                      // command buffer (+1 for null terminator)
    for (size_t i = 0; i < len; i++) {                  // copy characters
        cmd[i] = (char) buffer[start + i].character;    // copy character
    }
    cmd[len] = '\0';// null-terminate string

    // Function to convert string to integer

    int string_to_int(char* str, int* next_pos) {
        int res = 0;
        int i = 0;

        // Pula espaços se houver
        while (str[i] == ' ') i++;

        // Converte os caracteres '0'-'9' em valor numérico
        while (str[i] >= '0' && str[i] <= '9') {
            res = res * 10 + (str[i] - '0');
            i++;
        }

        if (next_pos) *next_pos = i; 
        return res;
    }

    // handle commands (compare manually to avoid relying on <string.h>)

    //listapp command
    if (cmd[0] == 'l' && cmd[1] == 'i' && cmd[2] == 's' && cmd[3] == 't' && cmd[4] == 'a' && cmd[5] == 'p' && cmd[6] == 'p') {
        char* args = &cmd[8]; // Pula "calc "


        if (args[0] == ' ' || args[1] == ' ') {
            print_str("\nUSAGE: listapp (command)!");
        }

        if (args[0] == 'v' && args[1] == 'e' && args[2] == 'r') {
            print_str("\nImagineOS ListApp V1.0");
        }

        if (args[0] == 'h' && args[1] == 'e' && args[2] == 'l' && args[3] == 'p') {
            print_str("\n");
            print_str("+----------------------------------------+\n");
            print_str("|      ImagineOS ListApp Help Guide      |\n");
            print_str("+----------------------------------------+\n");
            print_str("| [listapp apps] - Show all listapp apps |\n");
            print_str("| [listapp ver]  - Show listapp version  |\n");
            print_str("+----------------------------------------+\n");
        }

        if (args[0] == 'a' && args[1] == 'p' && args[2] == 'p' && args[3] == 's') {
            print_str("\n");
            print_str("+----------------------------------------+\n");
            print_str("|         ImagineOS Applications         |\n");
            print_str("+----------------------------------------+\n");
            print_str("| [calc] - Imagine Calculator            |\n");
            print_str("| [listapp ver]  - Show listapp version  |\n");
            print_str("+----------------------------------------+\n");
        }

        int pos = 0;
    }


    // Comando da Calculadora
    if (cmd[0] == 'c' && cmd[1] == 'a' && cmd[2] == 'l' && cmd[3] == 'c') {
        char* args = &cmd[5]; // Pula "calc "

        if (args[0] == 'v' && args[1] == 'e' && args[2] == 'r') {
            print_str("\nImagineOS Calculator V1.0");
            goto end_calc;
        }

        int pos = 0;

        // 1. Pega o primeiro número
        int n1 = string_to_int(args, &pos);
        
        // 2. O sinal está logo após o primeiro número
        char op = args[pos];
        
        // 3. Pega o segundo número (começando 1 posição após o sinal)
        int pos2 = 0;
        int n2 = string_to_int(&args[pos + 1], &pos2);

        int resultado = 0;

        // 4. Decide a conta baseada no sinal
        if (op == '+') resultado = n1 + n2;
        else if (op == '-') resultado = n1 - n2;
        else if (op == '*') resultado = n1 * n2;
        else if (op == '/') {
            if (n2 != 0) resultado = n1 / n2;
            else {
                print_str("\nErro: Divisao por zero!");
                goto end_calc;
            }
        }

        // 5. Exibe o resultado usando sua função de imprimir números
        print_str("\nResult: ");
        print_uint64_dec(resultado);

    end_calc:
        print_char('\n');
        shell_print_prompt();
        return;
    }

    if (len == 5) {                                     // length 5
        const char want_clear[5] = {'c','l','e','a','r'};     // clear
        int match_clear = 1;                                  // assume match
        for (size_t i = 0; i < 5; i++) {                // compare each char
            if (cmd[i] != want_clear[i]) { 
                match_clear = 0; break; 
            }// no match
        }
        if (match_clear) {                                    // if matched
            // Clear whole screen and reset cursor
            print_clear();                              // clear screen
            row = 0;                                    // reset row
            col = 0;                                    // reset col
            shell_print_prompt();                       // print new prompt
            return;                                     // done
        }
    }

    // reboot command
    if (len == 6) {                                     // length 6
        const char want_reboot[6] = {'r','e','b','o','o','t'};// reboot
        int match_reboot = 1;                                 // assume match
        for (size_t i = 0; i < 6; i++) {                      // compare each char
            if (cmd[i] != want_reboot[i]) {                   // no match
                match_reboot = 0; break;                      // no match
            }
        }
        if (match_reboot) {                             // if matched
            print_str("Rebooting...\n");                // feedback
            reboot_system();                            // reboot system
            // if reboot fails, loop
            for(;;);                                    // hang
        }
    }

    // Version Command
    if (len == 3){
        const char want_ver[3] = {'v','e','r'};
        int match_ver = 1;
        for (size_t i = 0; i < 3; i++) {
            if (cmd[i] != want_ver[i]){
                match_ver = 0; break;
            }
        }
        if (match_ver){
            print_str("\n");
            print_str("ImagineOS Alpha BUILD_20251225 - Development Preview\n");
            print_str("Copyright (C) 2025 The Imagine OS Project\n");
            print_str("Developed by: Adryan Alcantara <thenyxiecreator@yahoo.com>\n\n");
            print_set_color(PRINT_COLOR_BLACK, PRINT_COLOR_YELLOW);
            print_str("UNDER CONSTRUCTION!");
            print_set_color(PRINT_COLOR_WHITE, PRINT_COLOR_BLACK);
            print_str("\n");
        }
    }

    // shutdown command
    if (len == 4) {
        const char want_shutdown[4] = {'e','x','i','t'};
        int match_shutdown = 1;// assume match
        for (size_t i = 0; i < 4; i++/* compare each char */) {
            if (cmd[i] != want_shutdown[i]) {
                match_shutdown = 0; break;
            }
        }
        if (match_shutdown){
            print_str("Shutting down...\n");
            shutdown_system();
            for(;;); // hang if shutdown fails
        }
    }

    if (len == 5){
        const char want_hello[5] = {'h','e','l','l','o'};
        int match_hello = 1;
        for (size_t i = 0; i < 5; i++) {
            if (cmd[i] != want_hello[i]){
                match_hello = 0;
                break;
            }
        }
        if (match_hello){
            print_str("\nHello World!");
        }
    }

    if (len == 4){
        const char want_main[4] = {'m','a','i','n'};
        int match_main = 1;
        for (size_t i = 0; i < 4; i++) {
            if (cmd[i] != want_main[i]){
                match_main = 0;
                break;
            }
        }
        if (match_main){
            print_str("\n");
            print_str("ImagineOS Maintener Alpha 1.0\n");
            print_set_color(PRINT_COLOR_BLACK, PRINT_COLOR_YELLOW);
            print_str("This feature still not completed yet!");
            print_set_color(PRINT_COLOR_WHITE, PRINT_COLOR_BLACK);
            print_str("\n");
        }
    }

    //help command
    if (len == 4) {
        const char want_help[4] = {'h','e','l','p'};
        int match_help = 1;// assume match
        for (size_t i = 0; i < 4; i++/* compare each char */) {
            if (cmd[i] != want_help[i]) {
                match_help = 0; break;
            }
        }
        if (match_help){
            print_str("\n");
            print_str("+--------------------------------------------+\n");
            print_str("|         ImagineOS Help Guide V1.0          |\n");
            print_str("+--------------------------------------------+\n");
            print_str("| [help] - Display this screen.              |\n");
            print_str("| [hello] - Say hello!                       |\n");
            print_str("| [reboot] - Reboot the System.              |\n");
            print_str("| [clear] - Clear the Display.               |\n");
            print_str("| [ver] - Show the current version.          |\n");
            print_str("| [exit] - Turn off the system.              |\n");
            print_str("| [listapp] - ListApp Program List.          |\n");
            print_str("| [(PROG) ver] - Show the program version.   |\n");
            print_str("| [(PROG) help] - Show the program help.     |\n");
            print_str("+--------------------------------------------+");
        }
    }

    // default behaviour: print newline and new prompt
    print_char('\n');                                   // move to next line
    shell_print_prompt();                               // print new prompt
}

void shell_disable_cursor() {                           // disable hardware text cursor
    outportb(0x3D4, 10);                                // cursor start register
    outportb(0x3D5, 32);                                // set bit 5 to disable cursor
}

void set_cursor(size_t c, size_t r) {                   // set hardware text cursor position
    uint16_t pos = (uint16_t)(r * NUM_COLS + c);        // compute position
    outportb(0x3D4, 0x0E);                              // cursor high byte register
    outportb(0x3D5, (pos >> 8) & 0xFF);                 // send high byte
    outportb(0x3D4, 0x0F);                              // cursor low byte register
    outportb(0x3D5, pos & 0xFF);                        // send low byte
}

void enable_cursor(uint8_t start_scanline, 
    uint8_t end_scanline) {                             // enable hardware text cursor
    // ensure bit 5 is clear
    outportb(0x3D4, 0x0A);                              // cursor start register
    outportb(0x3D5, start_scanline & 0x1F);             // set start scanline
    outportb(0x3D4, 0x0B);                              // cursor end register
    outportb(0x3D5, end_scanline & 0x1F);               // set end scanline
}

void disable_cursor(void) {                             // disable hardware text cursor
    // set bit 5 to disable cursor
    outportb(0x3D4, 0x0A);                              // cursor start register
    uint8_t val = port_inb(0x3D5);                      // read current value
    outportb(0x3D4, 0x0A);                              // cursor start register
    outportb(0x3D5, val | 0x20);                        // set bit 5 to disable cursor
}

void toggle_cursor_visibility(void) {                   // toggle hardware text cursor visibility
    outportb(0x3D4, 0x0A);                              // cursor start register
    uint8_t val = port_inb(0x3D5);                      // read current value
    outportb(0x3D4, 0x0A);                              // cursor start register
    if (val & 0x20) {                                   // if bit 5 is set, cursor is disabled
        outportb(0x3D5, val & ~0x20);                   // enable cursor
    } else {                                            // else cursor is enabled
        outportb(0x3D5, val | 0x20);                    // disable cursor
    }
}

void reboot_system(void) {                              // reboot the system using keyboard controller
    // Try keyboard controller reset: wait input buffer empty, then send 0xFE
    for (int i = 0; i < 100; i++) {                     // wait up to some time
        uint8_t status = port_inb(0x64);                // keyboard controller status
        if ((status & 0x02) == 0) break;                // input buffer empty
    }
    port_outb(0x64, 0xFE);                              // pulse CPU reset line

    // Try the port 0x92 method as fallback
    uint8_t v = port_inb(0x92);                         // read port 0x92
    port_outb(0x92, v | 0x01);                          // set bit 0 to request reset

    // If still running, halt
    for (;;) { asm volatile("hlt"); }                   // halt CPU
}

void shutdown_system(void) {                            // shutdown the system using ACPI
    // Write to the ACPI PM1a control block to initiate shutdown
    // This is a simplified example and may not work on all hardware
    uint16_t pm1a_control_block = 0xB004;               // typical address, may vary
    uint16_t slp_typa = 0x2000;                         // SLP_TYP for S5 (soft off)
    uint16_t slp_en = 0x2000;                           // SLP_EN bit

    // Write the sleep type and enable bits to the PM1a control block
    port_outb(pm1a_control_block, (slp_typa | slp_en) & 0xFF);
    port_outb(pm1a_control_block + 1, ((slp_typa | slp_en) >> 8) & 0xFF);

    // If still running, halt
    print_clear(); // Just to avoid compiler warnings about unused functions
    print_str("You can turn off the computer now.\n");
    return;
    for (;;) { asm volatile("hlt"); }                   // halt CPU
}


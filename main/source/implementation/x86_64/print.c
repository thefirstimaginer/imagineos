// SOURCE_(x86_64)/source/implementation/x86_64/print.c
#include "print.h"
#include "shellMain.h"
#include "x86_64/port.h"
#include "x86_64/rtc.h"
#include "x86_64/graphics.h"   // para font_data e dimensões

#include <stdbool.h>

// VGA/text-mode characteristics
size_t NUM_COLS = 80;
size_t NUM_ROWS = 25;

// VGA text-mode character structure
struct Char {
    uint8_t character;
    uint8_t color;
};

// pointer para buffer de texto; em modo gráfico não é usado
struct Char* buffer = (struct Char*)0xb8000;

// cursor e cor atuais (text-mode)
size_t col = 0, row = 0;
uint8_t color = PRINT_COLOR_WHITE | (PRINT_COLOR_BLACK << 4);
size_t shell_prompt_col = 0, shell_prompt_row = 0;

// flag que indica se estamos desenhando texto sobre framebuffer
bool graphics_print = false;
uint8_t graph_fg = 15, graph_bg = 0;  // índices de cor 0‑15

// font_data está definido em graphics.c
extern const uint8_t font_data[256][FONT_HEIGHT];

/* funções de framebuffer declaradas em modules/video.c */
extern void planar_fill(uint8_t color);
extern void planar_set_pixel(int x, int y, uint8_t color);
extern void set_palette16();

// Clear a specific row in text mode
void clear_row(size_t row) {                        // clear a specific row
    struct Char empty = (struct Char) {             // empty character with current color
        character: ' ',                             // space character
        color: color,                               // current color
    };

    for (size_t col = 0; col < NUM_COLS; col++) {   // clear each column in the row
        buffer[col + NUM_COLS * row] = empty;       // set to empty
    }
}

// Clear the entire screen
void print_clear() {
    if (!graphics_print) {
        for (size_t i = 0; i < NUM_ROWS; i++) {
            clear_row(i);
        }
    } else {
        // em modo gráfico usa plano fill (driver vídeo deve fornecer)
        extern void planar_fill(uint8_t color);
        planar_fill(graph_bg);
        col = row = 0;
    }
}

void print_newline() {
    col = 0;

    if (!graphics_print) {
        if (row < NUM_ROWS - 1) {
            row++;
            set_cursor(col, row);
            return;
        }

        for (size_t r = 1; r < NUM_ROWS; r++) {
            for (size_t c = 0; c < NUM_COLS; c++) {
                struct Char character = buffer[c + NUM_COLS * r];
                buffer[c + NUM_COLS * (r - 1)] = character;
            }
        }

        clear_row(NUM_ROWS - 1);
        set_cursor(col, row);
    } else {
        // simples: se ultrapassar, limpa tudo
        if (row < NUM_ROWS - 1) {
            row++;
            return;
        }
        print_clear();
    }
}

void print_char(char character) {
    if (graphics_print) {
        // desenhar usando bitmap de fonte em framebuffer planar
        if (character == '\n') {
            print_newline();
            return;
        }
        if (col >= NUM_COLS) {
            print_newline();
        }
        // desenha caractere em (col,row)
        int base_x = col * FONT_WIDTH;
        int base_y = row * FONT_HEIGHT;
        const uint8_t *glyph = font_data[(uint8_t)character];
        for (int gy = 0; gy < FONT_HEIGHT; gy++) {
            for (int gx = 0; gx < FONT_WIDTH; gx++) {
                if (glyph[gy] & (1 << (7 - gx))) {
                    planar_set_pixel(base_x + gx, base_y + gy, graph_fg);
                } else {
                    planar_set_pixel(base_x + gx, base_y + gy, graph_bg);
                }
            }
        }
        col++;
        return;
    }

    // modo texto normal
    if (character == '\n') {
        print_newline();
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
    if (!graphics_print) {
        color = foreground + (background << 4);
    } else {
        graph_fg = foreground & 0x0F;
        graph_bg = background & 0x0F;
    }
}

void enable_graphics_print() {
    graphics_print = true;
    col = row = 0;
    // também redefinir cores para padrão
    graph_fg = 15;
    graph_bg = 0;
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

        if (!graphics_print) {
            // 3. Sobrescrever o caractere anterior com um espaço em branco
            buffer[col + NUM_COLS * row] = (struct Char) {
                character: ' ',
                color: color,
            };
            set_cursor(col, row);
        } else {
            // em gráfico, apenas reescreve o caractere com cor de fundo
            int base_x = col * FONT_WIDTH;
            int base_y = row * FONT_HEIGHT;
            for (int gy = 0; gy < FONT_HEIGHT; gy++) {
                for (int gx = 0; gx < FONT_WIDTH; gx++) {
                    planar_set_pixel(base_x + gx, base_y + gy, graph_bg);
                }
            }
        }
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

// SOURCE_(x86_64)/source/implementation/x86_64/print.c
#include "print.h"
#include "print_command.h"
#include "x86_64/port.h"
#include "x86_64/rtc.h"

// VGA text mode dimensions
size_t NUM_COLS = 80;// text mode dimensions
size_t NUM_ROWS = 25;// text mode dimensions

// VGA text mode character structure
struct Char {
    uint8_t character;  // ASCII character
    uint8_t color;      // color attribute
};

// VGA text mode buffer pointer
struct Char* buffer = (struct Char*) 0xb8000;// VGA text mode buffer address
#include "framebuffer.h"

// framebuffer color used by renderer (RGB 32-bit)
static uint32_t fb_fg = 0x00FFFFFF;// default white
static uint32_t fb_bg = 0x00000000;// default black

// If framebuffer is available, compute rows/cols from its size (assume 8x16 glyph cell)
static void recompute_size_from_fb() {  // recompute text size from framebuffer
    if (!fb_enabled) return;            // no framebuffer, do nothing
    const size_t gw = 10;               // glyph width
    const size_t gh = 16;               // glyph height
    NUM_COLS = fb_width / gw;           // number of columns
    NUM_ROWS = fb_height / gh;          // number of rows
}
// Current cursor position
size_t col = 0;
size_t row = 0;

// Current color attribute for text mode
uint8_t color = PRINT_COLOR_WHITE | PRINT_COLOR_BLACK << 4;
// Shell prompt tracking: start column and row of the editable area after the prompt
static size_t shell_prompt_col = 0;
static size_t shell_prompt_row = 0;

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
void print_clear() {                        // clear the entire screen
    if (fb_enabled) {                       // framebuffer mode
        recompute_size_from_fb();           // recompute size
        fb_clear(0x00000000);               // clear to black
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

void set_shell_color(uint8_t color) {
    // Se o seu sistema usa uma variável global para cor, atualize ela aqui
    // Exemplo: terminal_color = color;
    // Isso fará com que os próximos print_str usem a nova cor
}

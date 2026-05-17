#include "modules.h"
#include "print.h"
#include "x86_64/port.h"
#include <stdint.h>

void video_init() {
    // nada especial por enquanto
}

// função auxiliar para escrever par de registros VGA
static void vga_write_seq(uint8_t index, uint8_t value) {
    port_outb(0x3C4, index);
    port_outb(0x3C5, value);
}
static void vga_write_crtc(uint8_t index, uint8_t value) {
    port_outb(0x3D4, index);
    port_outb(0x3D5, value);
}
static void vga_write_graph(uint8_t index, uint8_t value) {
    port_outb(0x3CE, index);
    port_outb(0x3CF, value);
}
static void vga_write_attr(uint8_t index, uint8_t value) {
    (void)port_inb(0x3DA); /* reset flip‑flop */
    port_outb(0x3C0, index);
    port_outb(0x3C0, value);
}

// modo 0x13 manual (320x200x256) por programação direta das portas
static void vga_mode13() {
    /* existing 0x13 sequence unchanged */
    // misc
    port_outb(0x3C2, 0x63);
    // sequencer
    vga_write_seq(0x00, 0x03);
    vga_write_seq(0x01, 0x01);
    vga_write_seq(0x02, 0x0F);
    vga_write_seq(0x03, 0x00);
    vga_write_seq(0x04, 0x06);
    // CRTC
    vga_write_crtc(0x00, 0x5F);
    vga_write_crtc(0x01, 0x4F);
    vga_write_crtc(0x02, 0x50);
    vga_write_crtc(0x03, 0x82);
    vga_write_crtc(0x04, 0x54);
    vga_write_crtc(0x05, 0x80);
    vga_write_crtc(0x06, 0xBF);
    vga_write_crtc(0x07, 0x1F);
    vga_write_crtc(0x08, 0x00);
    vga_write_crtc(0x09, 0x41);
    vga_write_crtc(0x0A, 0x00);
    vga_write_crtc(0x0B, 0x00);
    vga_write_crtc(0x0C, 0x00);
    vga_write_crtc(0x0D, 0x00);
    vga_write_crtc(0x0E, 0x00);
    vga_write_crtc(0x0F, 0x00);
    vga_write_crtc(0x10, 0x9C);
    vga_write_crtc(0x11, 0x0E);
    vga_write_crtc(0x12, 0x8F);
    vga_write_crtc(0x13, 0x28);
    vga_write_crtc(0x14, 0x40);
    vga_write_crtc(0x15, 0x96);
    vga_write_crtc(0x16, 0xB9);
    vga_write_crtc(0x17, 0xA3);
    vga_write_crtc(0x18, 0xFF);
    // graphics controller
    vga_write_graph(0x00, 0x00);
    vga_write_graph(0x01, 0x00);
    vga_write_graph(0x02, 0x00);
    vga_write_graph(0x03, 0x00);
    vga_write_graph(0x04, 0x00);
    vga_write_graph(0x05, 0x40);
    vga_write_graph(0x06, 0x05);
    vga_write_graph(0x07, 0x0F);
    vga_write_graph(0x08, 0xFF);
    // attribute controller
    for (uint8_t i = 0; i < 0x20; ++i) {
        port_inb(0x3DA);
        port_outb(0x3C0, i);
        port_outb(0x3C0, i);
    }
    port_outb(0x3C0, 0x20); // finish attr
}

/* novo: configura modo 0x12 (640x480 16 cores planar) */
static void vga_mode12() {
    // ideia: usar registradores apropriados para 640x480
    port_outb(0x3C2, 0xE3);
    // sequencer
    vga_write_seq(0x00, 0x03);
    vga_write_seq(0x01, 0x01);
    vga_write_seq(0x02, 0x0F);
    vga_write_seq(0x03, 0x00);
    vga_write_seq(0x04, 0x0E);
    // CRTC (valores tomados da tabela de modo 0x12)
    vga_write_crtc(0x00, 0x5F);
    vga_write_crtc(0x01, 0x4F);
    vga_write_crtc(0x02, 0x50);
    vga_write_crtc(0x03, 0x82);
    vga_write_crtc(0x04, 0x54);
    vga_write_crtc(0x05, 0x80);
    vga_write_crtc(0x06, 0xBF);
    vga_write_crtc(0x07, 0x1F);
    vga_write_crtc(0x08, 0x00);
    vga_write_crtc(0x09, 0x41);
    vga_write_crtc(0x0A, 0x00);
    vga_write_crtc(0x0B, 0x00);
    vga_write_crtc(0x0C, 0x00);
    vga_write_crtc(0x0D, 0x00);
    vga_write_crtc(0x0E, 0x00);
    vga_write_crtc(0x0F, 0x00);
    vga_write_crtc(0x10, 0x9C);
    vga_write_crtc(0x11, 0x0E);
    vga_write_crtc(0x12, 0x8F);
    vga_write_crtc(0x13, 0x28);
    vga_write_crtc(0x14, 0x40);
    vga_write_crtc(0x15, 0x96);
    vga_write_crtc(0x16, 0xB9);
    vga_write_crtc(0x17, 0xA3);
    vga_write_crtc(0x18, 0xFF);
    // controlador gráfico: habilitar acesso aos planos
    vga_write_graph(0x00, 0x00);
    vga_write_graph(0x01, 0x00);
    vga_write_graph(0x02, 0x00);
    vga_write_graph(0x03, 0x00);
    vga_write_graph(0x04, 0x00);
    vga_write_graph(0x05, 0x10); // shift register write mask
    vga_write_graph(0x06, 0x05);
    vga_write_graph(0x07, 0x0F);
    vga_write_graph(0x08, 0xFF);
    // atributo permanece padrão
    for (uint8_t i = 0; i < 0x20; ++i) {
        port_inb(0x3DA);
        port_outb(0x3C0, i);
        port_outb(0x3C0, i);
    }
    port_outb(0x3C0, 0x20);
}

/* ajuda: escrever na memória planar de modo 0x12 */
void planar_fill(uint8_t color) {
    // cada plano ocupa 64KB consecutivos em 0xA0000
    for (int plane = 0; plane < 4; ++plane) {
        // escolher plano via sequencer mask
        port_outb(0x3C4, 0x02);
        port_outb(0x3C5, 1 << plane);
        uint8_t *fb = (uint8_t*)0xA0000;
        int bytes = (640 * 480) / 8; // cada byte contém 8 pixels
        uint8_t val = (color & (1 << plane)) ? 0xFF : 0x00;
        for (int i = 0; i < bytes; ++i) fb[i] = val;
    }
}

// preenche uma linha horizontal (y) com a cor indicada
void planar_fill_line(int y, uint8_t color) {
    unsigned base = y * 80; // bytes por linha no modo 0x12
    for (int plane = 0; plane < 4; ++plane) {
        port_outb(0x3C4, 0x02);
        port_outb(0x3C5, 1 << plane);
        uint8_t *fb = (uint8_t*)0xA0000 + base;
        uint8_t val = (color & (1 << plane)) ? 0xFF : 0x00;
        for (int i = 0; i < 80; i++) fb[i] = val;
    }
}

// configura um único pixel planar; x deve estar em [0,639], y em [0,479]
void planar_set_pixel(int x, int y, uint8_t color) {
    unsigned offset = (y * 80) + (x >> 3); // 80 bytes por linha
    uint8_t mask = 0x80 >> (x & 7);
    for (int plane = 0; plane < 4; ++plane) {
        port_outb(0x3C4, 0x02);
        port_outb(0x3C5, 1 << plane);
        uint8_t *fb = (uint8_t*)0xA0000;
        if (color & (1 << plane))
            fb[offset] |= mask;
        else
            fb[offset] &= ~mask;
    }
}

void set_palette16() {
    /* Cores padrão VGA 16‑cores em 6 bits por componente */
    static const uint8_t pal[16][3] = {
        {0,0,0},      {0,0,42},   {0,42,0},   {0,42,42},
        {42,0,0},     {42,0,42},  {42,21,0},  {42,42,42},
        {21,21,21},   {21,21,63}, {21,63,21}, {21,63,63},
        {63,21,21},   {63,21,63}, {63,63,21}, {63,63,63}
    };
    port_outb(0x3C8, 0);
    for (int i = 0; i < 16; i++) {
        port_outb(0x3C9, pal[i][0]);
        port_outb(0x3C9, pal[i][1]);
        port_outb(0x3C9, pal[i][2]);
    }
}

void video_run(char* args) {
    (void)args;
    print_str("[video] switching to mode 12 (640x480x16 planar)\n");
    vga_mode12();
    set_palette16();                      /* garante cores não pretas */
    /* habilita impressão gráfica (textos futuros usam framebuffer) */
    extern void enable_graphics_print();
    enable_graphics_print();

    /* desenha dois pontos de verificação nos cantos */
    planar_set_pixel(0, 0, 15);
    planar_set_pixel(639, 479, 15);

    /* exemplo de texto usando novo print: */
    print_str("graphics text enabled\n");

    /* teste simples: pintar cada linha com cor 0..15 repetidas */
    for (int y = 0; y < 480; y++) {
        uint8_t color = y % 16;
        planar_fill_line(y, color);
    }
    print_str("[video] done. type any key to continue (text lost)\n");
}

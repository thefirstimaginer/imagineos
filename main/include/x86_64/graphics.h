#ifndef GRAPHICS_H
#define GRAPHICS_H

#include <stdint.h>
#include <stddef.h>

// Definições para modo gráfico VESA 0x101 (640×480, 256 cores).
// assumimos framebuffer linear em 0xA0000; em produção, leia o endereço
// retornado pela função VESA 0x4F01. ocupa 640×480 = 307 200 bytes.

#define GRAPHICS_WIDTH 640
#define GRAPHICS_HEIGHT 480
#define GRAPHICS_BPP 8    // 8 bits por pixel (índice de cor)
#define GRAPHICS_FRAMEBUFFER ((uint8_t*)0xA0000)  // assumido linear

// valores de cor 0–255 – apenas alguns usados como convenção inicial
#define COLOR_BLACK     0x00
#define COLOR_WHITE     0xFF
#define COLOR_RED       0x04
#define COLOR_GREEN     0x02
#define COLOR_BLUE      0x01
#define COLOR_YELLOW    0x0E
#define COLOR_CYAN      0x03
#define COLOR_MAGENTA   0x05

// Estrutura para fonte bitmap (simples, 8x16 pixels por caractere)
#define FONT_WIDTH 8
#define FONT_HEIGHT 16
extern const uint8_t font_data[256][FONT_HEIGHT];  // Dados da fonte (definidos em graphics.c)

// Funções do driver gráfico
void graphics_init();  // Inicializa o modo gráfico
void graphics_clear(uint16_t color);  // Limpa a tela com uma cor
void graphics_put_pixel(int x, int y, uint16_t color);  // Desenha um pixel
void graphics_draw_char(int x, int y, char c, uint16_t fg_color, uint16_t bg_color);  // Desenha um caractere
void graphics_draw_string(int x, int y, const char* str, uint16_t fg_color, uint16_t bg_color);  // Desenha uma string

#endif</content>
<parameter name="filePath">/workspaces/imagine-os/main/include/graphics.h
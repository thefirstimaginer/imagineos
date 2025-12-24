/* Simple framebuffer interface */
#pragma once

#include <stdint.h>
#include <stddef.h>

extern int fb_enabled;
extern uint8_t *fb_addr;
extern uint32_t fb_width;
extern uint32_t fb_height;
extern uint32_t fb_pitch;
extern uint8_t fb_bpp;

// Initialize framebuffer from multiboot2 info pointer (address passed by GRUB)
void framebuffer_init_from_multiboot(uint64_t mb_info_addr);

void fb_clear(uint32_t color);
void fb_put_char_xy(char c, uint32_t x, uint32_t y, uint32_t fg, uint32_t bg);
void fb_putc(char c);
void fb_set_cursor_pos(size_t col, size_t row);
// Double-buffering and color control
void fb_flush();
void fb_set_colors(uint32_t fg, uint32_t bg);


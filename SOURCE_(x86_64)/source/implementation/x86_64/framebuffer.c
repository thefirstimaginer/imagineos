#include "framebuffer.h"
#include "print.h"
#include "x86_64/port.h"

#include <stdint.h>
#include <stddef.h>
#include "font8x16.h"

int fb_enabled = 0;
uint8_t *fb_addr = 0;
uint32_t fb_width = 0;
uint32_t fb_height = 0;
uint32_t fb_pitch = 0;
uint8_t fb_bpp = 0;
static uint32_t fb_fg_color = 0x00FFFFFF;
static uint32_t fb_bg_color = 0x00000000;

// Backbuffer maximum size (support up to 1024x768)
#define MAX_FB_W 1024
#define MAX_FB_H 768
static uint32_t fb_backbuffer[MAX_FB_W * MAX_FB_H];
static uint32_t *backbuffer = fb_backbuffer;

// very small (fake) glyph rendering: each character is an 8x8 cell.
// For now we render a filled 6x6 inner square for non-space characters.
static const uint32_t FONT_W = 8;
// We'll render glyphs as 8x16 by duplicating each 8x8 row twice
static const uint32_t FONT_H = 16;

static inline void put_pixel32_back(uint32_t x, uint32_t y, uint32_t color) {
    if (!fb_enabled) return;
    if (x >= fb_width || y >= fb_height) return;
    backbuffer[y * fb_width + x] = color;
}

static inline void put_pixel32(uint32_t x, uint32_t y, uint32_t color) {
    if (!fb_enabled) return;
    if (x >= fb_width || y >= fb_height) return;
    uint8_t *pixel = fb_addr + y * fb_pitch + x * 4;
    *((uint32_t*)pixel) = color;
}

void fb_clear(uint32_t color) {
    if (!fb_enabled) return;
    // clear backbuffer then flush
    for (uint32_t y = 0; y < fb_height; y++) {
        for (uint32_t x = 0; x < fb_width; x++) {
            backbuffer[y * fb_width + x] = color;
        }
    }
    fb_flush();
}

void fb_put_char_xy(char c, uint32_t x, uint32_t y, uint32_t fg, uint32_t bg) {
    if (!fb_enabled) return;
    // draw background box into backbuffer
    for (uint32_t yy = 0; yy < FONT_H; yy++) {
        for (uint32_t xx = 0; xx < FONT_W; xx++) {
            put_pixel32_back(x + xx, y + yy, bg);
        }
    }
    if ((unsigned char)c < 0x00 || (unsigned char)c > 0x7F) return;
    const uint8_t *glyph = font8x16[(unsigned char)c];
    for (uint32_t gy = 0; gy < FONT_H; gy++) {
        uint8_t row = glyph[gy];
        for (uint32_t gx = 0; gx < FONT_W; gx++) {
            if (row & (1 << (7 - gx))) {
                put_pixel32_back(x + gx, y + gy, fg);
            }
        }
    }
}

// simple state for cursor used by print.c
static size_t fb_col = 0;
static size_t fb_row = 0;

void fb_putc(char c) {
    if (!fb_enabled) return;
    if (c == '\n') {
        fb_col = 0;
        fb_row++;
        return;
    }
    uint32_t x = fb_col * FONT_W;
    uint32_t y = fb_row * FONT_H;
    fb_put_char_xy(c, x, y, fb_fg_color, fb_bg_color);
    fb_col++;
    if ((uint32_t)(fb_col * FONT_W) >= fb_width) { fb_col = 0; fb_row++; }
    // flush the backbuffer to screen for now (simple)
    fb_flush();
}

void fb_set_cursor_pos(size_t col, size_t row) {
    fb_col = col;
    fb_row = row;
}

// Multiboot2 parsing: look for tag type 8 (framebuffer)
struct mb2_tag { uint32_t type; uint32_t size; };

struct mb2_framebuffer_tag {
    struct mb2_tag tag;
    uint64_t addr;
    uint32_t pitch;
    uint32_t width;
    uint32_t height;
    uint8_t bpp;
    uint8_t type;
    uint16_t reserved;
    uint32_t red_field_position;
    uint32_t red_mask_size;
    uint32_t green_field_position;
    uint32_t green_mask_size;
    uint32_t blue_field_position;
    uint32_t blue_mask_size;
};

void framebuffer_init_from_multiboot(uint64_t mb_info_addr) {
    if (mb_info_addr == 0) return;
    uint8_t *ptr = (uint8_t*) (uintptr_t) mb_info_addr;
    uint32_t total_size = *((uint32_t*)ptr);
    // tags start after two uint32
    uint8_t *tagptr = ptr + 8;
    uint8_t *end = ptr + total_size;
    while (tagptr < end) {
        struct mb2_tag *tag = (struct mb2_tag*) tagptr;
        if (tag->type == 8) {
            struct mb2_framebuffer_tag *fb = (struct mb2_framebuffer_tag*) tagptr;
            fb_enabled = 1;
            fb_addr = (uint8_t*)(uintptr_t) fb->addr;
            fb_pitch = fb->pitch;
            fb_width = fb->width;
            fb_height = fb->height;
            fb_bpp = fb->bpp;
            // clamp backbuffer usage to supported size
            if (fb_width > MAX_FB_W || fb_height > MAX_FB_H) {
                fb_enabled = 0; // unsupported large mode
                return;
            }
            // clear screen black
            fb_clear(0x00000000);
                // Debug: print detected framebuffer size to text console
                print_str("[fb] detected: ");
                print_uint64_dec(fb_width);
                print_str("x");
                print_uint64_dec(fb_height);
                print_str(" bpp=");
                print_uint64_dec(fb_bpp);
                print_str("\n");
            return;
        }
        uint32_t size = tag->size;
        // tags are aligned to 8
        uint32_t aligned = (size + 7) & ~7;
        tagptr += aligned;
    }
}

void fb_flush() {
    if (!fb_enabled) return;
    // copy backbuffer rows to framebuffer memory respecting pitch
    for (uint32_t y = 0; y < fb_height; y++) {
        uint8_t *dst = fb_addr + y * fb_pitch;
        uint32_t *src = backbuffer + y * fb_width;
        // copy pixel by pixel
        for (uint32_t x = 0; x < fb_width; x++) {
            *((uint32_t*)(dst + x * 4)) = src[x];
        }
    }
}

void fb_set_colors(uint32_t fg, uint32_t bg) {
    fb_fg_color = fg;
    fb_bg_color = bg;
}

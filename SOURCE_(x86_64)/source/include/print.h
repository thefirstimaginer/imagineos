#pragma once

#include <stdint.h>
#include <stddef.h>

extern size_t col;
extern size_t row;

enum {
    PRINT_COLOR_BLACK = 0,
	PRINT_COLOR_BLUE = 1,
	PRINT_COLOR_GREEN = 2,
	PRINT_COLOR_CYAN = 3,
	PRINT_COLOR_RED = 4,
	PRINT_COLOR_MAGENTA = 5,
	PRINT_COLOR_BROWN = 6,
	PRINT_COLOR_LIGHT_GRAY = 7,
	PRINT_COLOR_DARK_GRAY = 8,
	PRINT_COLOR_LIGHT_BLUE = 9,
	PRINT_COLOR_LIGHT_GREEN = 10,
	PRINT_COLOR_LIGHT_CYAN = 11,
	PRINT_COLOR_LIGHT_RED = 12,
	PRINT_COLOR_PINK = 13,
	PRINT_COLOR_YELLOW = 14,
	PRINT_COLOR_WHITE = 15,
};

void print_clear();
void print_char(char character);
void print_str(char* string);
void print_set_color(uint8_t foreground, uint8_t background);
void print_uint64_dec(uint64_t value);
void print_uint64_hex(uint64_t value);
void print_uint64_bin(uint64_t value);
void backspace();
void shell_disable_cursor();
void shell_print_prompt();
void shell_handle_enter();
void reboot_system(void);
void shutdown_system(void);
void set_cursor(size_t col, size_t row);
void enable_cursor(uint8_t start_scanline, uint8_t end_scanline);
void disable_cursor(void);
void toggle_cursor_visibility(void);
void outportb(uint16_t port, uint8_t val);
void keyboard_keys_init();


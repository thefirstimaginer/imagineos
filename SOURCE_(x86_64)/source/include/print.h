// SOURCE_(x86_64)/source/include/print.h
#pragma once// print.h - Header file for print.c

#include <stdint.h>// for uint8_t, uint16_t, uint64_t
#include <stddef.h>// for size_t

extern size_t col;// current cursor position
extern size_t row;// current cursor position

enum {	// text colors
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

void print_clear();													// clear the screen
void print_char(char character);									// print single character
void print_str(char* string);										// print null-terminated string
void print_set_color(uint8_t foreground, uint8_t background);		// set text color
void print_uint64_dec(uint64_t value);								// print 64-bit value in decimal
void print_uint64_hex(uint64_t value);								// print 64-bit value in hexadecimal
void print_uint64_bin(uint64_t value);								// print 64-bit value in binary
void backspace();													// handle backspace key
void shell_disable_cursor();										// disable hardware text cursor
void shell_print_prompt();											// print shell prompt and set editable area
void shell_handle_enter();											// process command entered at prompt
void reboot_system(void);											// reboot the system using keyboard controller
void shutdown_system(void);											// shutdown the system using ACPI
void set_cursor(size_t col, size_t row);							// set hardware text cursor position
void enable_cursor(uint8_t start_scanline, uint8_t end_scanline);	// enable hardware text cursor
void disable_cursor(void);											// disable hardware text cursor
void toggle_cursor_visibility(void);								// toggle hardware text cursor visibility
void outportb(uint16_t port, uint8_t val);							// write a byte to the specified port
void keyboard_keys_init();											// initializes keyboard keys

/*
NOTA: Os tipos uint8_t, uint16_t, uint64_t e size_t são definidos em <stdint.h> e <stddef.h>,
e significam inteiros sem sinal de 8, 16, 64 bits e tamanho da plataforma, respectivamente.
*/


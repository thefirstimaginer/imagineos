#pragma once
#include "bool.h"

#include <stdint.h>

#define KEY_CODE_A 0x1E
#define KEY_CODE_B 0x30
#define KEY_CODE_C 0x2E
#define KEY_CODE_D 0x20
#define KEY_CODE_E 0x12
#define KEY_CODE_F 0x21
#define KEY_CODE_G 0x22
#define KEY_CODE_H 0x23
#define KEY_CODE_I 0x17
#define KEY_CODE_J 0x24
#define KEY_CODE_K 0x25
#define KEY_CODE_L 0x26
#define KEY_CODE_M 0x32
#define KEY_CODE_N 0x31
#define KEY_CODE_O 0x18
#define KEY_CODE_P 0x19
#define KEY_CODE_Q 0x10
#define KEY_CODE_R 0x13
#define KEY_CODE_S 0x1F
#define KEY_CODE_T 0x14
#define KEY_CODE_U 0x16
#define KEY_CODE_V 0x2F
#define KEY_CODE_W 0x11
#define KEY_CODE_X 0x2D
#define KEY_CODE_Y 0x15
#define KEY_CODE_Z 0x2C
// Números
#define KEY_CODE_1 0x02
#define KEY_CODE_2 0x03
#define KEY_CODE_3 0x04
#define KEY_CODE_4 0x05
#define KEY_CODE_5 0x06
#define KEY_CODE_6 0x07
#define KEY_CODE_7 0x08
#define KEY_CODE_8 0x09
#define KEY_CODE_9 0x0A
#define KEY_CODE_0 0x0B
//Números (Teclado Numérico)
#define KEY_CODE_NUMPAD_1 0xCF
#define KEY_CODE_NUMPAD_2 0xD0
#define KEY_CODE_NUMPAD_3 0xD1
#define KEY_CODE_NUMPAD_4 0xCB
#define KEY_CODE_NUMPAD_5 0xCC
#define KEY_CODE_NUMPAD_6 0xCD
#define KEY_CODE_NUMPAD_7 0xC7
#define KEY_CODE_NUMPAD_8 0xC8
#define KEY_CODE_NUMPAD_9 0xC9
#define KEY_CODE_NUMPAD_0 0xD2
#define KEY_CODE_NUMPAD_SLASH (0xE000 | 0x35)
#define KEY_CODE_NUMPAD_STAR 0x37
#define KEY_CODE_NUMPAD_MINUS 0x4A
#define KEY_CODE_NUMPAD_PLUS 0x4E
#define KEY_CODE_NUMPAD_PERIOD 0x53
#define KEY_CODE_NUMPAD_ENTER (0xE000 | 0x1C)
// Teclas de Confirmação
#define KEY_CODE_SPACE 0x39
#define KEY_CODE_ENTER 0x1C
#define KEY_CODE_BACKSPACE 0x0E
#define KEY_CODE_LSHIFT 0x2A
#define KEY_CODE_RSHIFT 0x36
#define KEY_CODE_NUM_LOCK 0x45
#define KEY_CODE_CAPS_LOCK 0x3A
#define KEY_CODE_SCROLL_LOCK 0x46
// Onde estão as definições dos KEY_CODEs
#define KEY_CODE_MINUS 0x0C
#define KEY_CODE_EQUALS 0x0D
#define KEY_CODE_LBRACKET 0x1A
#define KEY_CODE_RBRACKET 0x1B
#define KEY_CODE_APOSTROPHE 0x28
#define KEY_CODE_GRAVE 0x29
#define KEY_CODE_BACKSLASH 0x2B
#define KEY_CODE_PERIOD 0x34
#define KEY_CODE_SLASH 0x35
#define KEY_CODE_SEMICOLON 0x27
#define KEY_CODE_COMMA 0x33
// Setas do teclado
#define KEY_CODE_UP    0x48
#define KEY_CODE_DOWN  0x50
#define KEY_CODE_LEFT  0x4B
#define KEY_CODE_RIGHT 0x4D
enum {
	KEYBOARD_EVENT_TYPE_MAKE = 0,
	KEYBOARD_EVENT_TYPE_BREAK = 1,
};

struct KeyboardEvent {
	uint8_t type;
	uint16_t code;
	bool shift_active;
	bool num_lock_active;
	bool caps_active;
};

void keyboard_init();
void keyboard_set_handler(void (*handler)(struct KeyboardEvent event));

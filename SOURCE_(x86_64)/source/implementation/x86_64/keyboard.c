#include <stddef.h>
#include "bool.h"
#include "keyboard.h"
#include "x86_64/idt.h"
#include "x86_64/ps2.h"
#include "print.h"

#define KEYBOARD_EXTENDED_SCAN_CODE 0xE0
// Set to 1 to enable debug print of received scancodes (fat_code)
#define KEYBOARD_DEBUG 0

static bool is_shift_active = false;
static bool is_num_lock_active = false;
static bool is_caps_lock_active = false;
static bool is_scroll_lock_active = false;

void (*keyboard_handler_user)(struct KeyboardEvent event);

void keyboard_handler() {
	static bool is_extended = 0;
	
	uint8_t scan_code = ps2_read_scan_code();
	
	if (scan_code == KEYBOARD_EXTENDED_SCAN_CODE) {
		is_extended = true;
		return;
	}
	
	if (keyboard_handler_user == NULL) {
		return;
	}

#if KEYBOARD_DEBUG
	// Debug: print fat_code in hex to help mapping scancodes from VM
	{
		uint16_t preview_code = (uint16_t) (scan_code & 0xFF);
		print_str("scan: 0x");
		print_uint64_hex(preview_code);
		print_str(" fat: 0x");
		// fat_code not yet set for extended case; compute preview fat_code
		uint16_t fat_preview = (uint16_t)(scan_code & 0x7F);
		// if extended flag is set earlier, show that as well
		if (is_extended) {
			fat_preview |= (KEYBOARD_EXTENDED_SCAN_CODE << 8);
		}
		print_uint64_hex(fat_preview);
		print_str("\n");
	}
#endif
	
	uint16_t fat_code = scan_code & 0x7F;
	
	if (is_extended) {
		is_extended = false;
		fat_code |= KEYBOARD_EXTENDED_SCAN_CODE << 8;
	}
	
	// LÓGICA DE RASTREAMENTO DO SHIFT:
	if (fat_code == KEY_CODE_LSHIFT || fat_code == KEY_CODE_RSHIFT) {
		// Se o bit 0x80 NÃO está setado, é um 'make' (pressionou)
		if ((scan_code & 0x80) == 0) {
			is_shift_active = true;
		} 
		// Se o bit 0x80 ESTÁ setado, é um 'break' (soltou)
		else {
			is_shift_active = false;
		}
	}

	if (keyboard_handler_user == NULL) {
		return;
	}

	if (fat_code == KEY_CODE_NUM_LOCK) {
		if ((scan_code & 0x80) == 0) { // Verifica se é evento MAKE
			is_num_lock_active = !is_num_lock_active;
			// Atualiza LEDs
			uint8_t ledmask = (is_scroll_lock_active ? 0x01 : 0x00) | (is_num_lock_active ? 0x02 : 0x00) | (is_caps_lock_active ? 0x04 : 0x00);
			ps2_set_leds(ledmask);
		}
	}

	if (fat_code == KEY_CODE_CAPS_LOCK) {
		if ((scan_code & 0x80) == 0) { // MAKE
			is_caps_lock_active = !is_caps_lock_active;
			uint8_t ledmask = (is_scroll_lock_active ? 0x01 : 0x00) | (is_num_lock_active ? 0x02 : 0x00) | (is_caps_lock_active ? 0x04 : 0x00);
			ps2_set_leds(ledmask);
		}
	}

	if (fat_code == KEY_CODE_SCROLL_LOCK) {
		if ((scan_code & 0x80) == 0) { // MAKE
			is_scroll_lock_active = !is_scroll_lock_active;
			uint8_t ledmask = (is_scroll_lock_active ? 0x01 : 0x00) | (is_num_lock_active ? 0x02 : 0x00) | (is_caps_lock_active ? 0x04 : 0x00);
			ps2_set_leds(ledmask);
		}
	}

	struct KeyboardEvent event;
	
	if ((scan_code & 0x80) == 0) {
		event.type = KEYBOARD_EVENT_TYPE_MAKE;
	} else {
		event.type = KEYBOARD_EVENT_TYPE_BREAK;
	}
	
	event.code = fat_code;

	event.shift_active = is_shift_active;
	event.num_lock_active = is_num_lock_active;
	event.caps_active = is_caps_lock_active;
	
	keyboard_handler_user(event);
}

void keyboard_init() {
	idt_init();
	idt_set_handler_keyboard(keyboard_handler);
}

void keyboard_set_handler(void (*handler)(struct KeyboardEvent event)) {
	keyboard_handler_user = handler;	
}

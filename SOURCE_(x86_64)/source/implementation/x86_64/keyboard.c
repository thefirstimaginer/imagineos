#include <stddef.h>
#include "bool.h"
#include "keyboard.h"
#include "x86_64/idt.h"
#include "x86_64/ps2.h"
#include "print.h"

#define KEYBOARD_EXTENDED_SCAN_CODE 0xE0

static bool is_shift_active = false;
static bool is_num_lock_active = false;

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
			// Inverte o estado atual do Num Lock
			is_num_lock_active = !is_num_lock_active;

			print_str("Num Lock state: %d\n, !is_num_lock_active");

			// Opcional: Você pode enviar um comando para o controlador PS/2
			// para acender o LED do Num Lock se quiser, mas isso é mais avançado.
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
	
	keyboard_handler_user(event);
}

void keyboard_init() {
	idt_init();
	idt_set_handler_keyboard(keyboard_handler);
}

void keyboard_set_handler(void (*handler)(struct KeyboardEvent event)) {
	keyboard_handler_user = handler;	
}

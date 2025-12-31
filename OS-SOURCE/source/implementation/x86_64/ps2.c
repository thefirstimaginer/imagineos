#include "x86_64/port.h"

#include <stdint.h>

#define PORT_PS2_DATA 0x60
#define PORT_PS2_STATUS 0x64

#define PS2_CMD_SET_LEDS 0xED

uint8_t ps2_read_scan_code() {
	return port_inb(PORT_PS2_DATA);
}

// Write a byte to the PS/2 device, waiting for input buffer to be clear.
void ps2_write(uint8_t data) {
	// Wait until input buffer (bit 1) is clear
	while (port_inb(PORT_PS2_STATUS) & 0x02) {
		// busy wait
	}
	port_outb(PORT_PS2_DATA, data);
	port_wait();
}

// Set keyboard LED state: bit0=Scroll, bit1=Num, bit2=Caps
void ps2_set_leds(uint8_t mask) {
	ps2_write(PS2_CMD_SET_LEDS);
	ps2_write(mask & 0x07);
}

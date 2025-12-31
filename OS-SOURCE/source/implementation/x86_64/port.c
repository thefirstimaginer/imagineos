#include "x86_64/port.h"

void port_wait() {
	port_inb(0x80);
}

// Compatibility wrapper: some files call `outportb`; forward to `port_outb`.
void outportb(uint16_t port, uint8_t val) {
    port_outb(port, val);
}

#pragma once

#include <stdint.h>

uint8_t ps2_read_scan_code();
void ps2_write(uint8_t data);
void ps2_set_leds(uint8_t mask);

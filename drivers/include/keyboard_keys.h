#pragma once
#include "bool.h"

struct KeyboardEvent;
void handle_input(struct KeyboardEvent event);
char to_ascii(uint16_t code, bool shift_active, bool caps_active, bool num_lock_active);

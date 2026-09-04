//Version, works like neofetch
#include "print.h"
#include "string.h"
#include "modules.h"

void ver_init() {
	print_str("\n");
}

void ver_run(char* args) {
	ver_module_main(args);
}

void ver_module_main(char* args) {
	print_str("RRRRRR  1111\n");
	print_str("RR  RR    11\n");
	print_str("RRRR      11\n");
	print_str("RR  RR  111111\n");
	print_set_color(PRINT_COLOR_CYAN, PRINT_COLOR_BLACK);
	print_str("\n");
	print_str("Imagine Platform R1\n");
	print_str("(C) 2026 Imagine Project, All Rights Reserved.");
	print_set_color(PRINT_COLOR_WHITE, PRINT_COLOR_BLACK);
}
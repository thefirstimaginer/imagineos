//Version, works like neofetch
#include "print.h"
#include "string.h"

void ver_init() {
	print_str("\n");
}

void ver_run(char* args) {
	ver_module_main(args);
}

void ver_module_main(char* args) {
	print_str("IIIIII MM    MM  OOOO   SSSSS \n");
	print_str("  II   MMM  MMM OO  OO SS___  \n");
	print_str("  II   MM MM MM OO  OO     SS \n");
	print_str("IIIIII MM    MM  OOOO  SSSSS  \n");
	print_set_color(PRINT_COLOR_CYAN, PRINT_COLOR_BLACK);
	print_str("\n");
	print_str("ImagineOS System Shell R1\n");
	print_str("Copyright (C) 2026 Imagine Project, All Rights Reserved.");
	print_set_color(PRINT_COLOR_WHITE, PRINT_COLOR_BLACK);
}
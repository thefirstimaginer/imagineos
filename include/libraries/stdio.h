#pragma once

// Inclui as suas funções reais de impressão
#include "x86_64/print.h" 

// Define macros que a Lua espera encontrar
#define printf(fmt, ...) print_str(fmt) 
#define fprintf(file, fmt, ...) print_str(fmt)
#define fflush(file)
#define stdout (void*)1
#define stderr (void*)2

// Se você tiver uma função que imprime caracteres únicos:
#define putc(c, file) fb_putc(c)
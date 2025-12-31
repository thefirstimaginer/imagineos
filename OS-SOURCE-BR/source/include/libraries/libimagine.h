// LibImagine is the standard C header library for ImagineOS
#pragma once
#include "string.h"
#include "math.h"

#include <stddef.h>
#include <stdint.h>

// Memória
void* malloc(size_t size);
void* realloc(void* ptr, size_t size); // Lua usa muito realloc!
void free(void* ptr);

// Strings (essencial para nomes de variáveis na Lua)
size_t strlen(const char* s);
int strcmp(const char* s1, const char* s2);
void* memcpy(void* dest, const void* src, size_t n);
void* memset(void* s, int c, size_t n);

// Saída (ligado ao seu modo texto 0xb8000)
void printf(const char* format, ...);
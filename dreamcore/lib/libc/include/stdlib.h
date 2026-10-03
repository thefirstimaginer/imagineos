// "stdimagine.h" is the standard C library header for ImagineOS
#pragma once
#include "string.h"
#include "math.h"

#include <stddef.h>
#include <stdint.h>

// Memória
void* malloc(size_t size);
void* realloc(void* ptr, size_t size); // Lua usa muito realloc!
void free(void* ptr);

// Saída (ligado ao seu modo texto 0xb8000)
void printf(const char* format, ...);
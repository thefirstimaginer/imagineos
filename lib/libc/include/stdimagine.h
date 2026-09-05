#ifndef IMAGINEOS_STDIMAGINE_H
#define IMAGINEOS_STDIMAGINE_H

#include <string.h>
#include <math.h>
#include <stddef.h>
#include <stdint.h>

void *malloc(size_t size);
void *realloc(void *pointer, size_t size);
void free(void *pointer);
int printf(const char *format, ...);

#endif
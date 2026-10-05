#ifndef DREAMCORE_STDLIB_H
#define DREAMCORE_STDLIB_H

#include <stddef.h>

#define EXIT_SUCCESS 0
#define EXIT_FAILURE 1

_Noreturn void abort(void);
_Noreturn void exit(int status);
void *malloc(size_t size);
void *calloc(size_t count, size_t size);
void *realloc(void *pointer, size_t size);
void free(void *pointer);
int abs(int value);

#endif
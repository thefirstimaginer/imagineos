#ifndef DREAMCORE_STDIO_H
#define DREAMCORE_STDIO_H

#include <stdarg.h>

#define EOF (-1)

int printf(const char *format, ...);
int vprintf(const char *format, va_list arguments);
int puts(const char *text);
int putchar(int character);
int getchar(void);

#endif
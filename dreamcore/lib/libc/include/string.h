#ifndef IMAGINEOS_COMMON_STRING_H
#define IMAGINEOS_COMMON_STRING_H

#include <stddef.h>

int strcmp(const char *left, const char *right);
size_t strlen(const char *string);
char *strchr(const char *string, int character);
char *strcpy(char *destination, const char *source);
char *strncpy(char *destination, const char *source, size_t count);
void *memcpy(void *destination, const void *source, size_t count);
void *memmove(void *destination, const void *source, size_t count);
void *memset(void *destination, int value, size_t count);
char *skip_spaces(char *string);
int string_to_int(char *string, int *position);
void int_to_string(int value, char *string);

#endif
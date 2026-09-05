#pragma once

#include <stddef.h>

int strcmp(const char *s1, const char *s2);
void int_to_string(int n, char* str);
size_t strlen(const char* s);
char* strchr(const char* s, int c);
char* strcpy(char* dest, const char* src);
char* strncpy(char* dest, const char* src, size_t n);
void* memcpy(void* dest, const void* src, size_t n);
void* memmove(void* dest, const void* src, size_t n);
char* skip_spaces(char* s);
int string_to_int(char* s, int* pos);
void* memset(void* s, int c, size_t n);
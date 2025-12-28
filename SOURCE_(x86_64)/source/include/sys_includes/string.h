#pragma once

#include <stddef.h>

int strcmp(const char *s1, const char *s2);
void reverse(char* s);
void int_to_string(int n, char* str);
size_t strlen(const char* s);
char* strchr(const char* s, int c);
char* strncpy(char* dest, const char* src, size_t n);
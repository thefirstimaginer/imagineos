#include "string.h"

#include <stddef.h>

size_t strlen(const char* s) {
    size_t len = 0;
    while (s[len] != '\0') {
        len++;
    }
    return len;
}

int strcmp(const char* s1, const char* s2) {
    while (*s1 != '\0' && *s2 != '\0' && *s1 == *s2) {
        s1++;
        s2++;
    }
    if (*s1 == *s2) {
        return 0;
    }
    return (*(unsigned char*)s1 > *(unsigned char*)s2) ? 1 : -1;
}

char* strchr(const char* s, int c) {
    while (*s != '\0') {
        if (*s == (char)c) {
            return (char*)s;
        }
        s++;
    }
    if (c == '\0') {
        return (char*)s;
    }
    return 0;
}

char* strcpy(char* dest, const char* src) {
    char* result = dest;

    while ((*dest++ = *src++) != '\0') {
    }
    return result;
}

char* strncpy(char* dest, const char* src, size_t n) {
    size_t i = 0;
    while (i < n && src[i] != '\0') {
        dest[i] = src[i];
        i++;
    }
    while (i < n) {
        dest[i] = '\0';
        i++;
    }
    return dest;
}

void* memcpy(void* dest, const void* src, size_t n) {
    unsigned char* dest_bytes = (unsigned char*)dest;
    const unsigned char* src_bytes = (const unsigned char*)src;

    while (n--) {
        *dest_bytes++ = *src_bytes++;
    }
    return dest;
}

void* memmove(void* dest, const void* src, size_t n) {
    unsigned char* dest_bytes = (unsigned char*)dest;
    const unsigned char* src_bytes = (const unsigned char*)src;

    if (dest_bytes < src_bytes) {
        while (n--) {
            *dest_bytes++ = *src_bytes++;
        }
    } else if (dest_bytes > src_bytes) {
        dest_bytes += n;
        src_bytes += n;
        while (n--) {
            *--dest_bytes = *--src_bytes;
        }
    }
    return dest;
}

char* skip_spaces(char* s) {
    while (*s == ' ' || *s == '\t' || *s == '\n' || *s == '\r') {
        s++;
    }
    return s;
}

int string_to_int(char* s, int* pos) {
    int value = 0;
    int sign = 1;
    int index = 0;

    s = skip_spaces(s);
    if (*s == '-') {
        sign = -1;
        s++;
    }
    while (*s >= '0' && *s <= '9') {
        value = value * 10 + (*s - '0');
        s++;
        index++;
    }
    if (pos != 0) {
        *pos = index + (sign < 0 ? 1 : 0);
    }
    return sign * value;
}

void int_to_string(int n, char* str) {
    char buffer[32];
    int i = 0;
    int negative = 0;

    if (n < 0) {
        negative = 1;
        n = -n;
    }

    do {
        buffer[i++] = (char)('0' + (n % 10));
        n /= 10;
    } while (n > 0);

    if (negative) {
        buffer[i++] = '-';
    }

    int j = 0;
    while (i > 0) {
        str[j++] = buffer[--i];
    }
    str[j] = '\0';
}

void* memset(void* s, int c, size_t n) {
    unsigned char* p = (unsigned char*)s;
    while (n--) {
        *p++ = (unsigned char)c;
    }
    return s;
}

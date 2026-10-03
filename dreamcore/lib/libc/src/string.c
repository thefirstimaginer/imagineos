#include <string.h>

size_t strlen(const char *string) {
    size_t length = 0;
    while (string[length] != '\0') length++;
    return length;
}

int strcmp(const char *left, const char *right) {
    while (*left != '\0' && *left == *right) {
        left++;
        right++;
    }
    return (unsigned char)*left - (unsigned char)*right;
}

char *strchr(const char *string, int character) {
    while (*string != '\0') {
        if (*string == (char)character) return (char *)string;
        string++;
    }
    return character == '\0' ? (char *)string : NULL;
}

char *strcpy(char *destination, const char *source) {
    char *result = destination;
    while ((*destination++ = *source++) != '\0') { }
    return result;
}

char *strncpy(char *destination, const char *source, size_t count) {
    size_t index = 0;
    while (index < count && source[index] != '\0') {
        destination[index] = source[index];
        index++;
    }
    while (index < count) destination[index++] = '\0';
    return destination;
}

void *memcpy(void *destination, const void *source, size_t count) {
    unsigned char *to = destination;
    const unsigned char *from = source;
    while (count--) *to++ = *from++;
    return destination;
}

void *memmove(void *destination, const void *source, size_t count) {
    unsigned char *to = destination;
    const unsigned char *from = source;
    if (to < from) {
        while (count--) *to++ = *from++;
    } else if (to > from) {
        to += count;
        from += count;
        while (count--) *--to = *--from;
    }
    return destination;
}

void *memset(void *destination, int value, size_t count) {
    unsigned char *bytes = destination;
    while (count--) *bytes++ = (unsigned char)value;
    return destination;
}

char *skip_spaces(char *string) {
    while (*string == ' ' || *string == '\t' || *string == '\n' || *string == '\r') string++;
    return string;
}

int string_to_int(char *string, int *position) {
    int value = 0;
    int sign = 1;
    int consumed = 0;
    string = skip_spaces(string);
    if (*string == '-') {
        sign = -1;
        string++;
        consumed++;
    }
    while (*string >= '0' && *string <= '9') {
        value = value * 10 + (*string++ - '0');
        consumed++;
    }
    if (position != NULL) *position = consumed;
    return sign * value;
}

void int_to_string(int value, char *string) {
    char buffer[32];
    int index = 0;
    int negative = value < 0;
    if (negative) value = -value;
    do {
        buffer[index++] = (char)('0' + value % 10);
        value /= 10;
    } while (value != 0);
    if (negative) buffer[index++] = '-';
    while (index > 0) *string++ = buffer[--index];
    *string = '\0';
}
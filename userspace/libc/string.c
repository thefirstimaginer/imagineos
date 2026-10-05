#include <string.h>

void *memcpy(void *destination, const void *source, size_t count)
{
    unsigned char *output = destination;
    const unsigned char *input = source;
    for (size_t index = 0; index < count; index++) output[index] = input[index];
    return destination;
}

void *memmove(void *destination, const void *source, size_t count)
{
    unsigned char *output = destination;
    const unsigned char *input = source;
    if ((unsigned long)output <= (unsigned long)input) {
        for (size_t index = 0; index < count; index++) output[index] = input[index];
    } else {
        while (count) {
            count--;
            output[count] = input[count];
        }
    }
    return destination;
}

void *memset(void *destination, int value, size_t count)
{
    unsigned char *output = destination;
    for (size_t index = 0; index < count; index++) output[index] = (unsigned char)value;
    return destination;
}

int memcmp(const void *left, const void *right, size_t count)
{
    const unsigned char *first = left;
    const unsigned char *second = right;
    for (size_t index = 0; index < count; index++) {
        if (first[index] != second[index]) return (int)first[index] - (int)second[index];
    }
    return 0;
}

size_t strlen(const char *text)
{
    size_t length = 0;
    while (text[length]) length++;
    return length;
}

int strcmp(const char *left, const char *right)
{
    while (*left && (unsigned char)*left == (unsigned char)*right) {
        left++;
        right++;
    }
    return (int)(unsigned char)*left - (int)(unsigned char)*right;
}

int strncmp(const char *left, const char *right, size_t count)
{
    for (size_t index = 0; index < count; index++) {
        unsigned char first = (unsigned char)left[index];
        unsigned char second = (unsigned char)right[index];
        if (first != second || first == 0) return (int)first - (int)second;
    }
    return 0;
}

char *strcpy(char *destination, const char *source)
{
    char *result = destination;
    while ((*destination++ = *source++)) {}
    return result;
}

char *strncpy(char *destination, const char *source, size_t count)
{
    size_t index = 0;
    while (index < count && source[index]) {
        destination[index] = source[index];
        index++;
    }
    while (index < count) destination[index++] = 0;
    return destination;
}
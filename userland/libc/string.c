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

char *strchr(const char *text, int character)
{
    while (*text) {
        if ((unsigned char)*text == (unsigned char)character) return (char *)text;
        text++;
    }
    return character == 0 ? (char *)text : 0;
}

char *strrchr(const char *text, int character)
{
    const char *result = 0;
    while (*text) {
        if ((unsigned char)*text == (unsigned char)character) result = text;
        text++;
    }
    return character == 0 ? (char *)text : (char *)result;
}

char *strpbrk(const char *text, const char *accept)
{
    for (const char *cursor = text; *cursor; cursor++) {
        for (const char *selector = accept; *selector; selector++) {
            if (*cursor == *selector) return (char *)cursor;
        }
    }
    return 0;
}

char *strcat(char *destination, const char *source)
{
    char *result = destination + strlen(destination);
    strcpy(result, source);
    return destination;
}

int strcasecmp(const char *left, const char *right)
{
    while (*left && *right) {
        unsigned char first = (unsigned char)*left;
        unsigned char second = (unsigned char)*right;
        if (first >= 'A' && first <= 'Z') first += 'a' - 'A';
        if (second >= 'A' && second <= 'Z') second += 'a' - 'A';
        if (first != second) return (int)first - (int)second;
        left++;
        right++;
    }
    return (int)(unsigned char)*left - (int)(unsigned char)*right;
}

char *strdup(const char *text)
{
    size_t length = strlen(text) + 1;
    char *copy = malloc(length);
    if (!copy) return 0;
    memcpy(copy, text, length);
    return copy;
}

size_t strcspn(const char *text, const char *reject)
{
    size_t count = 0;
    while (text[count]) {
        for (const char *cursor = reject; *cursor; cursor++) {
            if (text[count] == *cursor) return count;
        }
        count++;
    }
    return count;
}

size_t strspn(const char *text, const char *accept)
{
    size_t count = 0;
    while (text[count]) {
        int match = 0;
        for (const char *cursor = accept; *cursor; cursor++) {
            if (text[count] == *cursor) {
                match = 1;
                break;
            }
        }
        if (!match) break;
        count++;
    }
    return count;
}

char *strtok(char *text, const char *delimiters)
{
    static char *cursor;
    char *result;
    if (text) cursor = text;
    if (!cursor) return 0;
    while (*cursor && strchr(delimiters, *cursor)) cursor++;
    if (!*cursor) return (cursor = 0);
    result = cursor;
    while (*cursor && !strchr(delimiters, *cursor)) cursor++;
    if (*cursor) {
        *cursor = 0;
        cursor++;
    }
    return result;
}

char *strerror(int number)
{
    (void)number;
    return "unknown error";
}

char *basename(char *path)
{
    char *last = strrchr(path, '/');
    return last ? last + 1 : path;
}

char *dirname(char *path)
{
    static char buffer[512];
    char *last = strrchr(path, '/');
    if (!last) {
        strcpy(buffer, ".");
        return buffer;
    }
    if (last == path) {
        strcpy(buffer, "/");
        return buffer;
    }
    size_t length = (size_t)(last - path);
    if (length >= sizeof(buffer)) length = sizeof(buffer) - 1;
    memcpy(buffer, path, length);
    buffer[length] = 0;
    return buffer;
}

#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define HEAP_CAPACITY (64 * 1024)
#define HEAP_ALIGNMENT 16

typedef union {
    size_t size;
    long double floating;
    void *pointer;
} AllocationHeader;

static unsigned char heap[HEAP_CAPACITY] __attribute__((aligned(HEAP_ALIGNMENT)));
static size_t heap_used;

void *malloc(size_t size)
{
    if (size == 0) size = 1;
    if (size > (size_t)-1 - sizeof(AllocationHeader) - HEAP_ALIGNMENT) return 0;
    size_t start = (heap_used + HEAP_ALIGNMENT - 1) & ~(HEAP_ALIGNMENT - 1);
    size_t total = sizeof(AllocationHeader) + size;
    if (start > HEAP_CAPACITY || total > HEAP_CAPACITY - start) return 0;
    AllocationHeader *header = (AllocationHeader *)(void *)(heap + start);
    header->size = size;
    heap_used = start + total;
    return header + 1;
}

void *calloc(size_t count, size_t size)
{
    if (size && count > (size_t)-1 / size) return 0;
    size_t total = count * size;
    void *allocation = malloc(total);
    if (allocation) memset(allocation, 0, total);
    return allocation;
}

void *realloc(void *pointer, size_t size)
{
    if (!pointer) return malloc(size);
    if (size == 0) return 0;
    AllocationHeader *header = (AllocationHeader *)pointer - 1;
    void *replacement = malloc(size);
    if (!replacement) return 0;
    size_t copied = header->size < size ? header->size : size;
    memcpy(replacement, pointer, copied);
    return replacement;
}

void free(void *pointer)
{
    (void)pointer;
}

int abs(int value)
{
    return value < 0 ? -value : value;
}

_Noreturn void abort(void)
{
    _exit(134);
}

_Noreturn void exit(int status)
{
    _exit(status);
}

int atoi(const char *text)
{
    int value = 0;
    int sign = 1;
    if (*text == '-') {
        sign = -1;
        text++;
    }
    while (*text >= '0' && *text <= '9') {
        value = value * 10 + (*text - '0');
        text++;
    }
    return value * sign;
}

unsigned long strtoul(const char *text, char **endptr, int base)
{
    unsigned long value = 0;
    unsigned long sign = 1;
    const char *cursor = text;
    while (*cursor == ' ' || *cursor == '\t' || *cursor == '\n' || *cursor == '\r') cursor++;
    if (*cursor == '-') {
        sign = (unsigned long)-1;
        cursor++;
    } else if (*cursor == '+') {
        cursor++;
    }
    while (*cursor) {
        unsigned char current = (unsigned char)*cursor;
        unsigned value_digit = 0;
        if (current >= '0' && current <= '9') value_digit = (unsigned)(current - '0');
        else if (base > 10 && current >= 'a' && current <= 'z') value_digit = (unsigned)(current - 'a' + 10);
        else if (base > 10 && current >= 'A' && current <= 'Z') value_digit = (unsigned)(current - 'A' + 10);
        else break;
        if (value_digit >= (unsigned)base) break;
        value = value * (unsigned long)base + value_digit;
        cursor++;
    }
    if (endptr) *endptr = (char *)cursor;
    if (sign == (unsigned long)-1) return (unsigned long)(-(long)value);
    return value;
}

unsigned long long strtoull(const char *text, char **endptr, int base)
{
    return (unsigned long long)strtoul(text, endptr, base);
}

char *getenv(const char *name)
{
    (void)name;
    return 0;
}

int putenv(const char *string)
{
    (void)string;
    return 0;
}

int unsetenv(const char *name)
{
    (void)name;
    return 0;
}

int setenv(const char *name, const char *value, int overwrite)
{
    (void)name;
    (void)value;
    (void)overwrite;
    return 0;
}

void *__errno_location(void)
{
    static int error = 0;
    return &error;
}

int *__errno(void)
{
    return (int *)__errno_location();
}

int sem_init(void *sem, int pshared, unsigned int value)
{
    (void)sem;
    (void)pshared;
    (void)value;
    return 0;
}

int sem_wait(void *sem)
{
    (void)sem;
    return 0;
}

int sem_post(void *sem)
{
    (void)sem;
    return 0;
}

int sigaction(int signum, const void *act, void *oldact)
{
    (void)signum;
    (void)act;
    (void)oldact;
    return 0;
}

void *dlopen(const char *filename, int flag)
{
    (void)filename;
    (void)flag;
    return 0;
}

void *dlsym(void *handle, const char *symbol)
{
    (void)handle;
    (void)symbol;
    return 0;
}

char *dlerror(void)
{
    return 0;
}

int dlclose(void *handle)
{
    (void)handle;
    return 0;
}

void __assert_fail(const char *expression, const char *file, unsigned int line, const char *function)
{
    (void)expression;
    (void)file;
    (void)line;
    (void)function;
    abort();
}

void __stack_chk_fail(void)
{
    abort();
}

void *realpath(const char *path, char *resolved_path)
{
    size_t length = strlen(path);
    if (!resolved_path) {
        resolved_path = malloc(length + 1);
        if (!resolved_path) return 0;
    }
    memcpy(resolved_path, path, length + 1);
    return resolved_path;
}

int setjmp(void *env)
{
    (void)env;
    return 0;
}

_Noreturn void longjmp(void *env, int value)
{
    (void)env;
    _exit(value ? value : 1);
}

unsigned long long __strtoull_internal(const char *text, char **endptr, int base)
{
    return strtoull(text, endptr, base);
}

unsigned long __strtoul_internal(const char *text, char **endptr, int base)
{
    return strtoul(text, endptr, base);
}

int __isoc23_strtol(const char *text, char **endptr, int base)
{
    return (int)strtoul(text, endptr, base);
}

unsigned long __isoc23_strtoul(const char *text, char **endptr, int base)
{
    return strtoul(text, endptr, base);
}

unsigned long long __isoc23_strtoull(const char *text, char **endptr, int base)
{
    return strtoull(text, endptr, base);
}

char *strndup(const char *text, size_t length)
{
    char *copy = malloc(length + 1);
    if (!copy) return 0;
    memcpy(copy, text, length);
    copy[length] = 0;
    return copy;
}

int vasprintf(char **buffer, const char *format, va_list arguments)
{
    char temporary[256];
    int result = vsnprintf(temporary, sizeof(temporary), format, arguments);
    if (result < 0) return result;
    *buffer = malloc((size_t)result + 1U);
    if (!*buffer) return -1;
    memcpy(*buffer, temporary, (size_t)result + 1U);
    return result;
}

int asprintf(char **buffer, const char *format, ...)
{
    va_list arguments;
    va_start(arguments, format);
    int result = vasprintf(buffer, format, arguments);
    va_end(arguments);
    return result;
}

char *index(const char *text, int character)
{
    return strchr(text, character);
}

char *rindex(const char *text, int character)
{
    return strrchr(text, character);
}

char *strcasestr(const char *haystack, const char *needle)
{
    size_t needle_length = strlen(needle);
    if (needle_length == 0) return (char *)haystack;
    for (size_t index = 0; haystack[index]; index++) {
        if (strncasecmp(haystack + index, needle, needle_length) == 0) return (char *)(haystack + index);
    }
    return 0;
}

int strncasecmp(const char *left, const char *right, size_t count)
{
    for (size_t index = 0; index < count; index++) {
        unsigned char first = (unsigned char)left[index];
        unsigned char second = (unsigned char)right[index];
        if (!first || !second) return (int)first - (int)second;
        if (first >= 'A' && first <= 'Z') first += 'a' - 'A';
        if (second >= 'A' && second <= 'Z') second += 'a' - 'A';
        if (first != second) return (int)first - (int)second;
    }
    return 0;
}

void *bsearch(const void *key, const void *base, size_t nmemb, size_t size, int (*compar)(const void *, const void *))
{
    (void)key;
    (void)base;
    (void)nmemb;
    (void)size;
    (void)compar;
    return 0;
}

int qsort(const void *base, size_t nmemb, size_t size, int (*compar)(const void *, const void *))
{
    (void)base;
    (void)nmemb;
    (void)size;
    (void)compar;
    return 0;
}

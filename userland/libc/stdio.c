#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define OUTPUT_CAPACITY 512

typedef struct {
    int fd;
} dreamcore_file_t;

FILE *stdin = (FILE *)0;
FILE *stdout = (FILE *)1;
FILE *stderr = (FILE *)2;

static int stream_write(FILE *stream, const void *buffer, size_t length)
{
    if (!stream) stream = stdout;
    if ((unsigned long)stream <= 2UL) {
        return write((int)(unsigned long)stream, buffer, length);
    }
    dreamcore_file_t *file = (dreamcore_file_t *)stream;
    return write(file->fd, buffer, length);
}

static int stream_read(FILE *stream, void *buffer, size_t length)
{
    if (!stream) stream = stdin;
    if ((unsigned long)stream <= 2UL) {
        return read((int)(unsigned long)stream, buffer, length);
    }
    dreamcore_file_t *file = (dreamcore_file_t *)stream;
    return read(file->fd, buffer, length);
}

static void emit_char(char *buffer, size_t *used, size_t capacity, char value)
{
    if (*used + 1 < capacity) {
        buffer[*used] = value;
        *used += 1;
    }
}

static void emit_string(char *buffer, size_t *used, size_t capacity, const char *text)
{
    while (*text) {
        emit_char(buffer, used, capacity, *text++);
    }
}

static void emit_number(char *buffer, size_t *used, size_t capacity,
                        unsigned long long value, unsigned base, int uppercase, int negative)
{
    char digits[32];
    unsigned index = 0;
    const char *alphabet = uppercase ? "0123456789ABCDEF" : "0123456789abcdef";
    do {
        digits[index++] = alphabet[value % base];
        value /= base;
    } while (value);
    if (negative) emit_char(buffer, used, capacity, '-');
    while (index) {
        emit_char(buffer, used, capacity, digits[--index]);
    }
}

static int format_write(char *buffer, size_t capacity, const char *format, va_list arguments)
{
    size_t used = 0;
    while (*format) {
        if (*format != '%') {
            emit_char(buffer, &used, capacity, *format++);
            continue;
        }
        format++;
        if (*format == '%') {
            emit_char(buffer, &used, capacity, '%');
            format++;
            continue;
        }
        switch (*format++) {
        case 'c': {
            char value = (char)va_arg(arguments, int);
            emit_char(buffer, &used, capacity, value);
            break;
        }
        case 's': {
            const char *text = va_arg(arguments, const char *);
            if (!text) text = "(null)";
            emit_string(buffer, &used, capacity, text);
            break;
        }
        case 'd':
        case 'i': {
            long long value = va_arg(arguments, long long);
            if (value < 0) {
                emit_number(buffer, &used, capacity, (unsigned long long)(-(value + 1)) + 1ULL, 10, 0, 1);
            } else {
                emit_number(buffer, &used, capacity, (unsigned long long)value, 10, 0, 0);
            }
            break;
        }
        case 'u': {
            unsigned int value = va_arg(arguments, unsigned int);
            emit_number(buffer, &used, capacity, value, 10, 0, 0);
            break;
        }
        case 'x':
        case 'X': {
            unsigned int value = va_arg(arguments, unsigned int);
            emit_number(buffer, &used, capacity, value, 16, *format == 'X' ? 1 : 0, 0);
            break;
        }
        case 'p': {
            void *value = va_arg(arguments, void *);
            unsigned long long address = (unsigned long long)(unsigned long)value;
            emit_string(buffer, &used, capacity, "0x");
            emit_number(buffer, &used, capacity, address, 16, 0, 0);
            break;
        }
        default:
            emit_char(buffer, &used, capacity, '%');
            emit_char(buffer, &used, capacity, format[-1]);
            break;
        }
    }
    if (capacity > 0) buffer[used < capacity - 1 ? used : capacity - 1] = 0;
    return (int)used;
}

static int flush_output(FILE *stream, const char *buffer, size_t length)
{
    if (!length) return 0;
    if (!stream) stream = stdout;
    if ((unsigned long)stream <= 2UL) {
        return write((int)(unsigned long)stream, buffer, length);
    }
    dreamcore_file_t *file = (dreamcore_file_t *)stream;
    return write(file->fd, buffer, length);
}

int vprintf(const char *format, va_list arguments)
{
    char buffer[OUTPUT_CAPACITY];
    int written = format_write(buffer, sizeof(buffer), format, arguments);
    return flush_output(stdout, buffer, (size_t)written);
}

int printf(const char *format, ...)
{
    va_list arguments;
    va_start(arguments, format);
    int result = vprintf(format, arguments);
    va_end(arguments);
    return result;
}

int puts(const char *text)
{
    size_t length = strlen(text);
    int wrote = flush_output(stdout, text, length);
    if (wrote < 0) return EOF;
    return flush_output(stdout, "\n", 1) < 0 ? EOF : 0;
}

int putchar(int character)
{
    char byte = (unsigned char)character;
    return flush_output(stdout, &byte, 1) == 1 ? character : EOF;
}

int getchar(void)
{
    unsigned char byte;
    long result = read(STDIN_FILENO, &byte, 1);
    return result == 1 ? (int)byte : EOF;
}

size_t fwrite(const void *buffer, size_t size, size_t count, FILE *stream)
{
    size_t total = size * count;
    if (!buffer || !total) return 0;
    return (size_t)(stream_write(stream, buffer, total) / (long)size);
}

size_t fread(void *buffer, size_t size, size_t count, FILE *stream)
{
    size_t total = size * count;
    if (!buffer || !total) return 0;
    long result = stream_read(stream, buffer, total);
    return result < 0 ? 0 : (size_t)result / size;
}

int fputc(int character, FILE *stream)
{
    char value = (char)character;
    return stream_write(stream, &value, 1) == 1 ? character : EOF;
}

int fputs(const char *text, FILE *stream)
{
    if (!text) return EOF;
    return stream_write(stream, text, strlen(text)) >= 0 ? 0 : EOF;
}

FILE *fopen(const char *path, const char *mode)
{
    (void)mode;
    int fd = open(path, 0, 0);
    if (fd < 0) return 0;
    dreamcore_file_t *file = malloc(sizeof(*file));
    if (!file) {
        close(fd);
        return 0;
    }
    file->fd = fd;
    return (FILE *)file;
}

int fclose(FILE *stream)
{
    if (!stream) return EOF;
    dreamcore_file_t *file = (dreamcore_file_t *)stream;
    int result = close(file->fd);
    free(file);
    return result == 0 ? 0 : EOF;
}

int fflush(FILE *stream)
{
    (void)stream;
    return 0;
}

int vfprintf(FILE *stream, const char *format, va_list arguments)
{
    char buffer[OUTPUT_CAPACITY];
    int written = format_write(buffer, sizeof(buffer), format, arguments);
    if (written < 0) return written;
    return flush_output(stream, buffer, (size_t)written);
}

int fprintf(FILE *stream, const char *format, ...)
{
    va_list arguments;
    va_start(arguments, format);
    int result = vfprintf(stream, format, arguments);
    va_end(arguments);
    return result;
}

int snprintf(char *buffer, size_t capacity, const char *format, ...)
{
    va_list arguments;
    va_start(arguments, format);
    int written = format_write(buffer, capacity, format, arguments);
    va_end(arguments);
    return written;
}

int vsnprintf(char *buffer, size_t capacity, const char *format, va_list arguments)
{
    return format_write(buffer, capacity, format, arguments);
}

int sprintf(char *buffer, const char *format, ...)
{
    va_list arguments;
    va_start(arguments, format);
    int written = vsnprintf(buffer, 4096, format, arguments);
    va_end(arguments);
    return written;
}

int vsprintf(char *buffer, const char *format, va_list arguments)
{
    return vsnprintf(buffer, 4096, format, arguments);
}

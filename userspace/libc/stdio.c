#include <stdio.h>
#include <unistd.h>

#define OUTPUT_CAPACITY 512

typedef struct {
    char bytes[OUTPUT_CAPACITY];
    size_t used;
    int written;
} Output;

static int flush(Output *output)
{
    if (!output->used) return 0;
    long result = write(STDOUT_FILENO, output->bytes, output->used);
    if (result < 0) return -1;
    output->written += (int)result;
    output->used = 0;
    return 0;
}

static int emit(Output *output, char character)
{
    if (output->used == OUTPUT_CAPACITY && flush(output) < 0) return -1;
    output->bytes[output->used++] = character;
    return 0;
}

static int emit_text(Output *output, const char *text)
{
    while (*text) {
        if (emit(output, *text++) < 0) return -1;
    }
    return 0;
}

static int emit_number(Output *output, unsigned long long value, unsigned base,
                       int uppercase, int negative, unsigned width, char padding)
{
    char digits[32];
    unsigned count = 0;
    const char *alphabet = uppercase ? "0123456789ABCDEF" : "0123456789abcdef";
    do {
        digits[count++] = alphabet[value % base];
        value /= base;
    } while (value);

    unsigned total = count + (unsigned)negative;
    while (total < width && padding == ' ') {
        if (emit(output, ' ') < 0) return -1;
        total++;
    }
    if (negative && emit(output, '-') < 0) return -1;
    while (total < width) {
        if (emit(output, '0') < 0) return -1;
        total++;
    }
    while (count) {
        if (emit(output, digits[--count]) < 0) return -1;
    }
    return 0;
}

int vprintf(const char *format, va_list arguments)
{
    Output output = {{0}, 0, 0};
    while (*format) {
        if (*format++ != '%') {
            if (emit(&output, format[-1]) < 0) return EOF;
            continue;
        }
        if (*format == '%') {
            format++;
            if (emit(&output, '%') < 0) return EOF;
            continue;
        }

        char padding = ' ';
        if (*format == '0') {
            padding = '0';
            format++;
        }
        unsigned width = 0;
        while (*format >= '0' && *format <= '9') {
            if (width < 128) width = width * 10 + (unsigned)(*format - '0');
            if (width > 128) width = 128;
            format++;
        }
        unsigned length = 0;
        if (*format == 'z') {
            length = 3;
            format++;
        } else if (*format == 'l') {
            length = 1;
            format++;
            if (*format == 'l') {
                length = 2;
                format++;
            }
        }

        char conversion = *format ? *format++ : 0;
        switch (conversion) {
        case 's': {
            const char *text = va_arg(arguments, const char *);
            if (!text) text = "(null)";
            if (emit_text(&output, text) < 0) return EOF;
            break;
        }
        case 'c':
            if (emit(&output, (char)va_arg(arguments, int)) < 0) return EOF;
            break;
        case 'd':
        case 'i': {
            long long value = length == 2 ? va_arg(arguments, long long)
                : length == 1 || length == 3 ? va_arg(arguments, long)
                : va_arg(arguments, int);
            int negative = value < 0;
            unsigned long long magnitude = negative
                ? 0ULL - (unsigned long long)value
                : (unsigned long long)value;
            if (emit_number(&output, magnitude, 10, 0, negative, width, padding) < 0) return EOF;
            break;
        }
        case 'u':
        case 'x':
        case 'X':
        case 'p': {
            unsigned long long value;
            if (conversion == 'p') value = (unsigned long long)(unsigned long)va_arg(arguments, void *);
            else if (length == 2) value = va_arg(arguments, unsigned long long);
            else if (length == 1 || length == 3) value = va_arg(arguments, unsigned long);
            else value = va_arg(arguments, unsigned int);
            if (emit_number(&output, value, conversion == 'u' ? 10 : 16,
                            conversion == 'X', 0, width, padding) < 0) return EOF;
            break;
        }
        default:
            if (emit(&output, '%') < 0) return EOF;
            if (conversion && emit(&output, conversion) < 0) return EOF;
            break;
        }
    }
    if (flush(&output) < 0) return EOF;
    return output.written;
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
    if (printf("%s\n", text) < 0) return EOF;
    return 0;
}

int putchar(int character)
{
    unsigned char byte = (unsigned char)character;
    return write(STDOUT_FILENO, &byte, 1) == 1 ? byte : EOF;
}

int getchar(void)
{
    unsigned char byte;
    return read(STDIN_FILENO, &byte, 1) == 1 ? byte : EOF;
}
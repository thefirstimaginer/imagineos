#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <sys/syscall.h>
#include <unistd.h>

int putchar(int character) {
    char value = (char)character;
    return write(STDOUT_FILENO, &value, 1) == 1 ? character : -1;
}

int puts(const char *string) {
    size_t length = strlen(string);
    if (write(STDOUT_FILENO, string, length) != (ssize_t)length) return -1;
    return putchar('\n') == -1 ? -1 : 0;
}

int printf(const char *format, ...) {
    va_list arguments;
    int written = 0;
    va_start(arguments, format);
    while (*format != '\0') {
        if (*format != '%') {
            if (putchar((unsigned char)*format++) == -1) { written = -1; break; }
            written++;
            continue;
        }
        format++;
        if (*format == 's') {
            const char *string = va_arg(arguments, const char *);
            size_t length = strlen(string);
            if (write(STDOUT_FILENO, string, length) != (ssize_t)length) { written = -1; break; }
            written += (int)length;
        } else if (*format == 'c') {
            if (putchar(va_arg(arguments, int)) == -1) { written = -1; break; }
            written++;
        } else if (*format == '%') {
            if (putchar('%') == -1) { written = -1; break; }
            written++;
        } else {
            if (putchar('%') == -1 || putchar((unsigned char)*format) == -1) { written = -1; break; }
            written += 2;
        }
        format++;
    }
    va_end(arguments);
    return written;
}
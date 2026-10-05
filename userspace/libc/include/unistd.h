#ifndef DREAMCORE_UNISTD_H
#define DREAMCORE_UNISTD_H

#include <stddef.h>

typedef long ssize_t;

#define STDIN_FILENO 0
#define STDOUT_FILENO 1
#define STDERR_FILENO 2

extern int errno;

ssize_t read(int descriptor, void *buffer, size_t count);
ssize_t write(int descriptor, const void *buffer, size_t count);
void _exit(int status) __attribute__((noreturn));

#endif
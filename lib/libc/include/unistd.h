#ifndef IMAGINEOS_LIBC_UNISTD_H
#define IMAGINEOS_LIBC_UNISTD_H

#include <stddef.h>

typedef long ssize_t;

ssize_t read(int fd, void *buffer, size_t count);
ssize_t write(int fd, const void *buffer, size_t count);
void exit(int status) __attribute__((noreturn));

#endif
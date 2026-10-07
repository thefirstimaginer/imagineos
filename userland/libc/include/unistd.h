#ifndef DREAMCORE_UNISTD_H
#define DREAMCORE_UNISTD_H

#include <stddef.h>

#include <sys/stat.h>

typedef long ssize_t;
typedef long off_t;
typedef unsigned short mode_t;

#define STDIN_FILENO 0
#define STDOUT_FILENO 1
#define STDERR_FILENO 2

#define SEEK_SET 0
#define SEEK_CUR 1
#define SEEK_END 2

extern int errno;

ssize_t read(int descriptor, void *buffer, size_t count);
ssize_t write(int descriptor, const void *buffer, size_t count);
int open(const char *path, int flags, ...);
int close(int descriptor);
off_t lseek(int descriptor, off_t offset, int whence);
int stat(const char *path, struct stat *buffer);
int fstat(int descriptor, struct stat *buffer);
void _exit(int status) __attribute__((noreturn));

#endif
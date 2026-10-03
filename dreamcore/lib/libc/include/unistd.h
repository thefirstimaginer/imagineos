#ifndef IMAGINEOS_LIBC_UNISTD_H
#define IMAGINEOS_LIBC_UNISTD_H


#define STDOUT_FILENO 1
#define STDIN_FILENO  0
#define STDERR_FILENO 2


#include <stddef.h>

typedef long ssize_t;

ssize_t read(int fd, void *buffer, size_t count);
ssize_t read_nonblock(int fd, void *buffer, size_t count);
ssize_t write(int fd, const void *buffer, size_t count);
unsigned long get_ticks(void);
int exec_service(const char *service_name);
long fork(void);
long waitpid(long pid, int *status, int options);
void exit(int status) __attribute__((noreturn));

#endif
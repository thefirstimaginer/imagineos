#include <sys/syscall.h>
#include <errno.h>
#include <unistd.h>
#include "syscall_numbers.h"

static ssize_t syscall_result(ssize_t result) {
    if (result >= 0) return result;
    errno = (int)-result;
    return -1;
}

ssize_t read(int fd, void *buffer, size_t count) {
    return syscall_result(syscall6(SYS_READ, fd, (long)buffer, (long)count, 0, 0, 0));
}

ssize_t read_nonblock(int fd, void *buffer, size_t count) {
    return syscall_result(syscall6(SYS_READ_NONBLOCK, fd, (long)buffer,
        (long)count, 0, 0, 0));
}

ssize_t write(int fd, const void *buffer, size_t count) {
    return syscall_result(syscall6(SYS_WRITE, fd, (long)buffer, (long)count, 0, 0, 0));
}

unsigned long get_ticks(void) {
    return (unsigned long)syscall_result(syscall6(SYS_GET_TICKS,
        0, 0, 0, 0, 0, 0));
}

int exec_service(const char *service_name) {
    return (int)syscall_result(syscall6(SYS_EXEC_SERVICE,
        (long)service_name, 0, 0, 0, 0, 0));
}

long fork(void) {
    return syscall_result(syscall6(SYS_FORK, 0, 0, 0, 0, 0, 0));
}

long waitpid(long pid, int *status, int options) {
    return syscall_result(syscall6(SYS_WAITPID, pid, (long)status,
        options, 0, 0, 0));
}

void exit(int status) {
    syscall6(SYS_EXIT, status, 0, 0, 0, 0, 0);
    for (;;) __asm__ volatile ("hlt");
}
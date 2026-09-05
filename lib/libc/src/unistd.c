#include <sys/syscall.h>
#include <unistd.h>

ssize_t read(int fd, void *buffer, size_t count) {
    return syscall6(SYS_READ, fd, (long)buffer, (long)count, 0, 0, 0);
}

ssize_t write(int fd, const void *buffer, size_t count) {
    return syscall6(SYS_WRITE, fd, (long)buffer, (long)count, 0, 0, 0);
}

void exit(int status) {
    syscall6(SYS_EXIT, status, 0, 0, 0, 0, 0);
    for (;;) __asm__ volatile ("hlt");
}
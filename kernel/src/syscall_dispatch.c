#include <stddef.h>
#include <stdint.h>
#include "syscall.h"
#include "process.h"
#include "print.h"
#include <syscall_numbers.h>

static long sys_write(int fd, const char *buffer, size_t count) {
    size_t index;
    if (fd != STDOUT_FILENO && fd != STDERR_FILENO) return -1;
    if (buffer == NULL) return -1;
    for (index = 0; index < count; index++) print_char(buffer[index]);
    return (long)count;
}

static long sys_exit(int status) __attribute__((noreturn));

static long sys_exit(int status) {
    (void)status;
    if (current_process != NULL) current_process->state = PROCESS_TERMINATED;
    for (;;) __asm__ volatile ("hlt");
}

long kernel_syscall_handler(long number, long arg1, long arg2, long arg3,
                            long arg4, long arg5, long arg6) {
    (void)arg4;
    (void)arg5;
    (void)arg6;
    switch (number) {
        case SYS_WRITE: return sys_write((int)arg1, (const char *)arg2, (size_t)arg3);
        case SYS_EXIT: return sys_exit((int)arg1);
        default: return -1;
    }
}
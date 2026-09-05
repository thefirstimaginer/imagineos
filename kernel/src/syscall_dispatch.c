#include <stddef.h>
#include <stdint.h>
#include "syscall.h"
#include "process.h"
#include "print.h"
#include "input.h"
#include "user.h"
#include <syscall_numbers.h>

static long sys_write(int fd, const char *buffer, size_t count) {
    size_t index;
    if (fd != STDOUT_FILENO && fd != STDERR_FILENO) return -EBADF;
    if (buffer == NULL) return -EFAULT;
    for (index = 0; index < count; index++) print_char(buffer[index]);
    return (long)count;
}

static long sys_read(int fd, char *buffer, size_t count) {
    if (fd != STDIN_FILENO) return -EBADF;
    if (buffer == NULL) return -EFAULT;
    if (count == 0) return 0;
    return input_read(buffer, count);
}

static long sys_exec_service(const char *service_name) {
    if (service_name == NULL) return -EFAULT;
    return user_exec_service(service_name);
}

static long sys_exit(int status) __attribute__((noreturn));

static long sys_exit(int status) {
    (void)status;
    if (current_process != NULL) current_process->state = PROCESS_TERMINATED;
    for (;;) __asm__ volatile ("hlt");
}

long kernel_syscall_handler(long number, long arg1, long arg2, long arg3) {
    switch (number) {
        case SYS_READ: return sys_read((int)arg1, (char *)arg2, (size_t)arg3);
        case SYS_WRITE: return sys_write((int)arg1, (const char *)arg2, (size_t)arg3);
        case SYS_EXEC_SERVICE: return sys_exec_service((const char *)arg1);
        case SYS_EXIT: return sys_exit((int)arg1);
        default: return -ENOSYS;
    }
}
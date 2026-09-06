#include <stddef.h>
#include <stdint.h>
#include "syscall.h"
#include "process.h"
#include "print.h"
#include "input.h"
#include "user.h"
#include "scheduler.h"
#include "paging.h"
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

static long sys_read_nonblock(int fd, char *buffer, size_t count) {
    if (fd != STDIN_FILENO) return -EBADF;
    if (buffer == NULL) return -EFAULT;
    return input_read_nonblock(buffer, count);
}

static long sys_fork(void) {
    Process *child;
    uint64_t cr3 = paging_create_user_space();

    if (cr3 == 0 || current_process == NULL) return -EAGAIN;
    paging_copy_user_image(cr3);
    child = process_create_user("child", cr3);
    if (child == NULL) return -EAGAIN;
    child->state = PROCESS_READY;
    child->user_frame = current_process->user_frame;
    child->user_frame.rax = 0;
    current_process->user_frame.rax = child->pid;
    return (long)child->pid;
}

static long sys_waitpid(long pid, int *status) {
    Process *child = process_find((uint32_t)pid);

    if (child == NULL || child->parent_pid != current_process->pid) return -ECHILD;
    if (child->state != PROCESS_TERMINATED) {
        current_process->state = PROCESS_BLOCKED;
        current_process->waiting_for_pid = (uint32_t)pid;
        current_process->waiting_status = (uint64_t)(uintptr_t)status;
        child->state = PROCESS_RUNNING;
        current_process = child;
        user_resume(&child->user_frame, child->context.cr3);
    }
    if (status != NULL) *status = child->exit_status;
    process_reap(child);
    current_process->waiting_for_pid = 0;
    return (long)child->pid;
}

static long sys_exit(int status) __attribute__((noreturn));

static long sys_exit(int status) {
    Process *parent;

    if (current_process == NULL) for (;;) __asm__ volatile("hlt");
    process_mark_exit(current_process, status);
    parent = process_find(current_process->parent_pid);
    if (parent != NULL && parent->state == PROCESS_BLOCKED) {
        uint32_t child_pid = current_process->pid;

        paging_activate(parent->context.cr3);
        if (parent->waiting_status != 0) {
            *(int *)(uintptr_t)parent->waiting_status = status;
        }
        process_reap(current_process);
        parent->state = PROCESS_RUNNING;
        parent->waiting_for_pid = 0;
        parent->waiting_status = 0;
        parent->user_frame.rax = child_pid;
        current_process = parent;
        print_str("[INFO] exit resume pid=");
        print_uint64_dec(parent->pid);
        print_str(" parent=");
        print_uint64_dec(parent->parent_pid);
        print_str(" rip=0x");
        print_uint64_hex(parent->user_frame.rip);
        print_str(" rsp=0x");
        print_uint64_hex(parent->user_frame.rsp);
        print_str(" cr3=0x");
        print_uint64_hex(parent->context.cr3);
        print_str("\n");
        user_resume(&parent->user_frame, parent->context.cr3);
    }
    for (;;) __asm__ volatile ("hlt");
}

long kernel_syscall_handler(long number, long arg1, long arg2, long arg3) {
    process_capture_user_frame();
    switch (number) {
        case SYS_READ: return sys_read((int)arg1, (char *)arg2, (size_t)arg3);
        case SYS_WRITE: return sys_write((int)arg1, (const char *)arg2, (size_t)arg3);
        case SYS_EXEC_SERVICE: return sys_exec_service((const char *)arg1);
        case SYS_READ_NONBLOCK: return sys_read_nonblock((int)arg1, (char *)arg2, (size_t)arg3);
        case SYS_GET_TICKS: return (long)scheduler_ticks();
        case SYS_CLEAR_TERMINAL: print_clear(); return 0;
        case SYS_FORK: return sys_fork();
        case SYS_WAITPID: return sys_waitpid((long)arg1, (int *)arg2);
        case SYS_EXIT: return sys_exit((int)arg1);
        default: return -ENOSYS;
    }
}
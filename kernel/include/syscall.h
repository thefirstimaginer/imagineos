#ifndef IMAGINEOS_KERNEL_SYSCALL_H
#define IMAGINEOS_KERNEL_SYSCALL_H

#include <stdint.h>

void syscall_init(void);
long kernel_syscall_handler(long number, long arg1, long arg2, long arg3);

#endif
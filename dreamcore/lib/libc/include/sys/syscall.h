#ifndef IMAGINEOS_LIBC_SYS_SYSCALL_H
#define IMAGINEOS_LIBC_SYS_SYSCALL_H

#include <syscall_numbers.h>

long syscall6(long number, long arg1, long arg2, long arg3,
              long arg4, long arg5, long arg6);

#endif
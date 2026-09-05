#include <sys/syscall.h>

long syscall6(long number, long arg1, long arg2, long arg3,
              long arg4, long arg5, long arg6) {
    long result;
    register long register_arg4 __asm__("r10") = arg4;
    register long register_arg5 __asm__("r8") = arg5;
    register long register_arg6 __asm__("r9") = arg6;
    __asm__ volatile (
        "syscall"
        : "=a"(result)
        : "a"(number), "D"(arg1), "S"(arg2), "d"(arg3),
          "r"(register_arg4), "r"(register_arg5), "r"(register_arg6)
        : "rcx", "r11", "memory");
    return result;
}
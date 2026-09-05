#include <sys/syscall.h>
#include <syscall_numbers.h>

static long sys_clear_terminal(void) {
    long result;
    register long arg4 __asm__("r10") = 0;
    register long arg5 __asm__("r8") = 0;
    register long arg6 __asm__("r9") = 0;

    __asm__ volatile (
        "syscall"
        : "=a"(result)
        : "a"(SYS_CLEAR_TERMINAL), "D"(0), "S"(0), "d"(0),
          "r"(arg4), "r"(arg5), "r"(arg6)
        : "rcx", "r11", "memory");

    return result;
}

int main(void) {
    sys_clear_terminal();
    return 0;
}

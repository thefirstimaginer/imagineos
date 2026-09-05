global syscall_entry
extern kernel_syscall_handler
extern user_kernel_stack

section .bss
align 8
syscall_user_stack: resq 1

section .text
bits 64
syscall_entry:
    mov [rel syscall_user_stack], rsp
    mov rsp, user_kernel_stack + 4096
    push r11
    push rcx
    mov rcx, rdx
    mov rdx, rsi
    mov rsi, rdi
    mov rdi, rax
    call kernel_syscall_handler
    pop rcx
    pop r11
    mov rsp, [rel syscall_user_stack]
    sysret
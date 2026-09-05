global syscall_entry
extern kernel_syscall_handler
extern user_kernel_stack
global syscall_user_frame

section .bss
align 8
syscall_user_frame: resq 18
syscall_user_stack: resq 1

section .text
bits 64
syscall_entry:
    mov [rel syscall_user_frame + 0], rax
    mov [rel syscall_user_frame + 8], rbx
    mov [rel syscall_user_frame + 16], rcx
    mov [rel syscall_user_frame + 24], rdx
    mov [rel syscall_user_frame + 32], rsi
    mov [rel syscall_user_frame + 40], rdi
    mov [rel syscall_user_frame + 48], rbp
    mov [rel syscall_user_frame + 56], r8
    mov [rel syscall_user_frame + 64], r9
    mov [rel syscall_user_frame + 72], r10
    mov [rel syscall_user_frame + 80], r11
    mov [rel syscall_user_frame + 88], r12
    mov [rel syscall_user_frame + 96], r13
    mov [rel syscall_user_frame + 104], r14
    mov [rel syscall_user_frame + 112], r15
    mov [rel syscall_user_frame + 120], rcx
    mov [rel syscall_user_frame + 128], rsp
    mov [rel syscall_user_frame + 136], r11
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
    mov rbx, [rel syscall_user_frame + 8]
    mov rdx, [rel syscall_user_frame + 24]
    mov rsi, [rel syscall_user_frame + 32]
    mov rdi, [rel syscall_user_frame + 40]
    mov rbp, [rel syscall_user_frame + 48]
    mov r8, [rel syscall_user_frame + 56]
    mov r9, [rel syscall_user_frame + 64]
    mov r10, [rel syscall_user_frame + 72]
    mov r12, [rel syscall_user_frame + 88]
    mov r13, [rel syscall_user_frame + 96]
    mov r14, [rel syscall_user_frame + 104]
    mov r15, [rel syscall_user_frame + 112]
    push qword 0x1B
    push qword [rel syscall_user_frame + 128]
    push qword [rel syscall_user_frame + 136]
    push qword 0x23
    push qword [rel syscall_user_frame + 120]
    iretq
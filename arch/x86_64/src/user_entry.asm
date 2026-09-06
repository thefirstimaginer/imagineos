global user_enter
global user_kernel_stack

section .bss
align 16
user_kernel_stack: resb 4096

section .text
bits 64
user_enter:
    cli
    mov rax, rsp
    mov cr3, rdx
    mov rsp, rsi
    push qword 0x1B
    push rsi
    push qword 0x202
    push qword 0x23
    push rdi
    iretq
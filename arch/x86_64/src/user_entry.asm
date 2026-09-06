global user_enter
global user_resume
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

user_resume:
    cli
    mov r15, rdi
    mov cr3, rsi
    push qword 0x1B
    push qword [r15 + 128]
    push qword [r15 + 136]
    push qword 0x23
    push qword [r15 + 120]
    mov rax, [r15 + 0]
    mov rbx, [r15 + 8]
    mov rcx, [r15 + 16]
    mov rdx, [r15 + 24]
    mov rsi, [r15 + 32]
    mov rdi, [r15 + 40]
    mov rbp, [r15 + 48]
    mov r8, [r15 + 56]
    mov r9, [r15 + 64]
    mov r10, [r15 + 72]
    mov r11, [r15 + 80]
    mov r12, [r15 + 88]
    mov r13, [r15 + 96]
    mov r14, [r15 + 104]
    mov r15, [r15 + 112]
    iretq
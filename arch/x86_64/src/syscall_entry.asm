global syscall_entry
extern kernel_syscall_handler

section .text
bits 64
syscall_entry:
    push r11
    push rcx
    push r9
    push r8
    push r10
    push rdx
    push rsi
    push rdi
    push rax

    mov rdi, [rsp]
    mov rsi, [rsp + 8]
    mov rdx, [rsp + 16]
    mov rcx, [rsp + 24]
    mov r8,  [rsp + 32]
    mov r9,  [rsp + 40]
    sub rsp, 8
    mov rax, [rsp + 56]
    mov [rsp], rax
    call kernel_syscall_handler
    add rsp, 8

    pop rax
    pop rdi
    pop rsi
    pop rdx
    pop r10
    pop r8
    pop r9
    pop rcx
    pop r11
    sysretq
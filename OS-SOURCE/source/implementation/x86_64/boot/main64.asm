global long_mode_start
extern kernel_main

section .text
bits 64
long_mode_start:
    ; load null into all data segment registers
    mov ax, 0
    mov ss, ax
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    ; pass multiboot info pointer from EBX (now RBX) in RDI
    mov rdi, rbx
    call kernel_main
    call enable_sse
    hlt

enable_sse:
    ; Verificar suporte a SSE (opcional, mas recomendado)
    ; Habilitar SSE
    mov rax, cr0
    and ax, 0xFFFB      ; Limpar coprocessor emulation CR0.EM
    or ax, 0x2          ; Setar coprocessor monitoring CR0.MP
    mov cr0, rax
    mov rax, cr4
    or ax, 3 << 9       ; Setar CR4.OSFXSR e CR4.OSXMMEXCPT
    mov cr4, rax
    ret


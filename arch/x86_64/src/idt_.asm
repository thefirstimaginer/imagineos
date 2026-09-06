extern idt_handler_keyboard
extern idt_handler_timer
extern idt_handler_timer_frame
global idt_load

idt_load:
	lidt [rdi]
	ret

%macro WRAPPED_HANDLER 1
	global %1_wrapped
	
	%1_wrapped:
		; save general-purpose registers
		push rax
		push rbx
		push rcx
		push rdx
		push rbp
		push rsi
		push rdi
		push r8
		push r9
		push r10
		push r11
		push r12
		push r13
		push r14
		push r15

		call %1

		; restore general-purpose registers
		pop r15
		pop r14
		pop r13
		pop r12
		pop r11
		pop r10
		pop r9
		pop r8
		pop rdi
		pop rsi
		pop rbp
		pop rdx
		pop rcx
		pop rbx
		pop rax
		
		iretq
%endmacro

WRAPPED_HANDLER idt_handler_keyboard

global idt_handler_timer_wrapped
idt_handler_timer_wrapped:
	push rax
	push rbx
	push rcx
	push rdx
	push rbp
	push rsi
	push rdi
	push r8
	push r9
	push r10
	push r11
	push r12
	push r13
	push r14
	push r15
	mov rdi, rsp
	call idt_handler_timer_frame
	pop r15
	pop r14
	pop r13
	pop r12
	pop r11
	pop r10
	pop r9
	pop r8
	pop rdi
	pop rsi
	pop rbp
	pop rdx
	pop rcx
	pop rbx
	pop rax
	iretq

%macro EXCEPTION_HANDLER 1
global idt_exception_%1_wrapped
idt_exception_%1_wrapped:
	mov rsi, rsp
	mov edi, %1
call idt_handle_exception
.halt:
	cli
	hlt
	jmp .halt
%endmacro

extern idt_handle_exception
EXCEPTION_HANDLER 0
EXCEPTION_HANDLER 1
EXCEPTION_HANDLER 2
EXCEPTION_HANDLER 3
EXCEPTION_HANDLER 4
EXCEPTION_HANDLER 5
EXCEPTION_HANDLER 6
EXCEPTION_HANDLER 7
EXCEPTION_HANDLER 8
EXCEPTION_HANDLER 9
EXCEPTION_HANDLER 10
EXCEPTION_HANDLER 11
EXCEPTION_HANDLER 12
EXCEPTION_HANDLER 13
EXCEPTION_HANDLER 14
EXCEPTION_HANDLER 15
EXCEPTION_HANDLER 16
EXCEPTION_HANDLER 17
EXCEPTION_HANDLER 18
EXCEPTION_HANDLER 19
EXCEPTION_HANDLER 20
EXCEPTION_HANDLER 21
EXCEPTION_HANDLER 22
EXCEPTION_HANDLER 23
EXCEPTION_HANDLER 24
EXCEPTION_HANDLER 25
EXCEPTION_HANDLER 26
EXCEPTION_HANDLER 27
EXCEPTION_HANDLER 28
EXCEPTION_HANDLER 29
EXCEPTION_HANDLER 30
EXCEPTION_HANDLER 31

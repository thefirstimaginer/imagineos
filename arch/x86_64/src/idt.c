#include <stddef.h>
#include <stdint.h>
#include "gdt.h"
#include "idt.h"
#include "pic.h"
#include "scheduler.h"
#include "print.h"

#define IDT_IRQ0_TIMER 0x20
#define IDT_IRQ1_KEYBOARD 0x21

#define IDT_GATE_PRESENT (1 << 7)
#define IDT_GATE_DPL0 (0b00 << 5)
#define IDT_GATE_DPL1 (0b01 << 5)
#define IDT_GATE_DPL2 (0b10 << 5)
#define IDT_GATE_DPL3 (0b11 << 5)
#define IDT_GATE_TYPE_INTERRUPT 0xE

#define IDT_ENTRY_TYPE_INTERRUPT (IDT_GATE_PRESENT | IDT_GATE_DPL0 | IDT_GATE_TYPE_INTERRUPT)

struct IdtEntry {
	uint16_t offset_low;
	uint16_t selector;
	uint8_t  ist;
	uint8_t  type;
	uint16_t offset_mid;
	uint32_t offset_high;
	uint32_t reserved;
} __attribute__((packed));

struct IdtPtr {
	uint16_t limit;
	uint64_t base;
} __attribute__((packed));

struct IdtEntry idt[256] __attribute__((aligned(16)));
struct IdtPtr idt_ptr;

void (*idt_handler_keyboard_user)();

#define EXCEPTION_COUNT 32

#define DECLARE_EXCEPTION_HANDLER(number) \
	extern void idt_exception_##number##_wrapped(void)

DECLARE_EXCEPTION_HANDLER(0);
DECLARE_EXCEPTION_HANDLER(1);
DECLARE_EXCEPTION_HANDLER(2);
DECLARE_EXCEPTION_HANDLER(3);
DECLARE_EXCEPTION_HANDLER(4);
DECLARE_EXCEPTION_HANDLER(5);
DECLARE_EXCEPTION_HANDLER(6);
DECLARE_EXCEPTION_HANDLER(7);
DECLARE_EXCEPTION_HANDLER(8);
DECLARE_EXCEPTION_HANDLER(9);
DECLARE_EXCEPTION_HANDLER(10);
DECLARE_EXCEPTION_HANDLER(11);
DECLARE_EXCEPTION_HANDLER(12);
DECLARE_EXCEPTION_HANDLER(13);
DECLARE_EXCEPTION_HANDLER(14);
DECLARE_EXCEPTION_HANDLER(15);
DECLARE_EXCEPTION_HANDLER(16);
DECLARE_EXCEPTION_HANDLER(17);
DECLARE_EXCEPTION_HANDLER(18);
DECLARE_EXCEPTION_HANDLER(19);
DECLARE_EXCEPTION_HANDLER(20);
DECLARE_EXCEPTION_HANDLER(21);
DECLARE_EXCEPTION_HANDLER(22);
DECLARE_EXCEPTION_HANDLER(23);
DECLARE_EXCEPTION_HANDLER(24);
DECLARE_EXCEPTION_HANDLER(25);
DECLARE_EXCEPTION_HANDLER(26);
DECLARE_EXCEPTION_HANDLER(27);
DECLARE_EXCEPTION_HANDLER(28);
DECLARE_EXCEPTION_HANDLER(29);
DECLARE_EXCEPTION_HANDLER(30);
DECLARE_EXCEPTION_HANDLER(31);

extern void idt_load(struct IdtPtr* idt_ptr);

void idt_set_entry(uint8_t vector, uint64_t isr_addr, uint16_t selector, uint8_t type) {
	idt[vector] = (struct IdtEntry) {
		.offset_low = (uint16_t) (isr_addr >> 0),
		.selector = selector,
		.ist = 0,
		.type = type,
		.offset_mid = (uint16_t) (isr_addr >> 16),
		.offset_high = (uint32_t) (isr_addr >> 32),
		.reserved = 0,
	};
}

extern void idt_handler_keyboard_wrapped();

void idt_handler_keyboard() {
	if (idt_handler_keyboard_user != NULL) {
		idt_handler_keyboard_user();
	}
	
	pic_eoi_master();
}

extern void idt_handler_timer_wrapped();

void idt_handler_timer() {
	extern void scheduler_tick();
	scheduler_tick();
}

void idt_handler_timer_frame(InterruptFrame *frame) {
	scheduler_user_tick(frame);
}

void idt_handle_exception(uint64_t vector, uint64_t *stack) {
	uint64_t error_code = 0;
	uint64_t rip;
	uint64_t cs;
	uint64_t rflags;
	uint64_t rsp = 0;
	uint64_t ss = 0;

	if (vector == 8 || vector == 10 || vector == 11 || vector == 12 ||
		vector == 13 || vector == 14 || vector == 17 || vector == 21 ||
		vector == 29 || vector == 30) {
		error_code = stack[0];
		rip = stack[1];
		cs = stack[2];
		rflags = stack[3];
		if ((cs & 3) != 0) {
			rsp = stack[4];
			ss = stack[5];
		}
	} else {
		rip = stack[0];
		cs = stack[1];
		rflags = stack[2];
		if ((cs & 3) != 0) {
			rsp = stack[3];
			ss = stack[4];
		}
	}
	print_str("\n[KERNEL EXCEPTION] vector=0x");
	print_uint64_hex(vector);
	print_str(" error=0x");
	print_uint64_hex(error_code);
	print_str(" rip=0x");
	print_uint64_hex(rip);
	print_str(" cs=0x");
	print_uint64_hex(cs);
	print_str(" rflags=0x");
	print_uint64_hex(rflags);
	print_str(" rsp=0x");
	print_uint64_hex(rsp);
	print_str(" ss=0x");
	print_uint64_hex(ss);
	print_str(" cr3=0x");
	{
		uint64_t cr3;
		__asm__ volatile ("mov %%cr3, %0" : "=r"(cr3));
		print_uint64_hex(cr3);
	}
	if (vector == 14) {
		uint64_t fault_address;
		__asm__ volatile ("mov %%cr2, %0" : "=r"(fault_address));
		print_str(" cr2=0x");
		print_uint64_hex(fault_address);
	}
	print_str("\nSystem halted.\n");
	__asm__ volatile ("cli");
	for (;;) __asm__ volatile ("hlt");
}

void idt_init() {
	pic_remap();
	
	idt_ptr.limit = (sizeof(struct IdtEntry) * 256) - 1;
	idt_ptr.base = (uint64_t) &idt;
	
	idt_set_entry(IDT_IRQ0_TIMER, (uint64_t) idt_handler_timer_wrapped, GDT_SELECTOR_CS_KERNEL, IDT_ENTRY_TYPE_INTERRUPT);
	idt_set_entry(IDT_IRQ1_KEYBOARD, (uint64_t) idt_handler_keyboard_wrapped, GDT_SELECTOR_CS_KERNEL, IDT_ENTRY_TYPE_INTERRUPT);
	idt_set_entry(0, (uint64_t) idt_exception_0_wrapped, GDT_SELECTOR_CS_KERNEL, IDT_ENTRY_TYPE_INTERRUPT);
	idt_set_entry(1, (uint64_t) idt_exception_1_wrapped, GDT_SELECTOR_CS_KERNEL, IDT_ENTRY_TYPE_INTERRUPT);
	idt_set_entry(2, (uint64_t) idt_exception_2_wrapped, GDT_SELECTOR_CS_KERNEL, IDT_ENTRY_TYPE_INTERRUPT);
	idt_set_entry(3, (uint64_t) idt_exception_3_wrapped, GDT_SELECTOR_CS_KERNEL, IDT_ENTRY_TYPE_INTERRUPT);
	idt_set_entry(4, (uint64_t) idt_exception_4_wrapped, GDT_SELECTOR_CS_KERNEL, IDT_ENTRY_TYPE_INTERRUPT);
	idt_set_entry(5, (uint64_t) idt_exception_5_wrapped, GDT_SELECTOR_CS_KERNEL, IDT_ENTRY_TYPE_INTERRUPT);
	idt_set_entry(6, (uint64_t) idt_exception_6_wrapped, GDT_SELECTOR_CS_KERNEL, IDT_ENTRY_TYPE_INTERRUPT);
	idt_set_entry(7, (uint64_t) idt_exception_7_wrapped, GDT_SELECTOR_CS_KERNEL, IDT_ENTRY_TYPE_INTERRUPT);
	idt_set_entry(8, (uint64_t) idt_exception_8_wrapped, GDT_SELECTOR_CS_KERNEL, IDT_ENTRY_TYPE_INTERRUPT);
	idt_set_entry(9, (uint64_t) idt_exception_9_wrapped, GDT_SELECTOR_CS_KERNEL, IDT_ENTRY_TYPE_INTERRUPT);
	idt_set_entry(10, (uint64_t) idt_exception_10_wrapped, GDT_SELECTOR_CS_KERNEL, IDT_ENTRY_TYPE_INTERRUPT);
	idt_set_entry(11, (uint64_t) idt_exception_11_wrapped, GDT_SELECTOR_CS_KERNEL, IDT_ENTRY_TYPE_INTERRUPT);
	idt_set_entry(12, (uint64_t) idt_exception_12_wrapped, GDT_SELECTOR_CS_KERNEL, IDT_ENTRY_TYPE_INTERRUPT);
	idt_set_entry(13, (uint64_t) idt_exception_13_wrapped, GDT_SELECTOR_CS_KERNEL, IDT_ENTRY_TYPE_INTERRUPT);
	idt_set_entry(14, (uint64_t) idt_exception_14_wrapped, GDT_SELECTOR_CS_KERNEL, IDT_ENTRY_TYPE_INTERRUPT);
	idt_set_entry(15, (uint64_t) idt_exception_15_wrapped, GDT_SELECTOR_CS_KERNEL, IDT_ENTRY_TYPE_INTERRUPT);
	idt_set_entry(16, (uint64_t) idt_exception_16_wrapped, GDT_SELECTOR_CS_KERNEL, IDT_ENTRY_TYPE_INTERRUPT);
	idt_set_entry(17, (uint64_t) idt_exception_17_wrapped, GDT_SELECTOR_CS_KERNEL, IDT_ENTRY_TYPE_INTERRUPT);
	idt_set_entry(18, (uint64_t) idt_exception_18_wrapped, GDT_SELECTOR_CS_KERNEL, IDT_ENTRY_TYPE_INTERRUPT);
	idt_set_entry(19, (uint64_t) idt_exception_19_wrapped, GDT_SELECTOR_CS_KERNEL, IDT_ENTRY_TYPE_INTERRUPT);
	idt_set_entry(20, (uint64_t) idt_exception_20_wrapped, GDT_SELECTOR_CS_KERNEL, IDT_ENTRY_TYPE_INTERRUPT);
	idt_set_entry(21, (uint64_t) idt_exception_21_wrapped, GDT_SELECTOR_CS_KERNEL, IDT_ENTRY_TYPE_INTERRUPT);
	idt_set_entry(22, (uint64_t) idt_exception_22_wrapped, GDT_SELECTOR_CS_KERNEL, IDT_ENTRY_TYPE_INTERRUPT);
	idt_set_entry(23, (uint64_t) idt_exception_23_wrapped, GDT_SELECTOR_CS_KERNEL, IDT_ENTRY_TYPE_INTERRUPT);
	idt_set_entry(24, (uint64_t) idt_exception_24_wrapped, GDT_SELECTOR_CS_KERNEL, IDT_ENTRY_TYPE_INTERRUPT);
	idt_set_entry(25, (uint64_t) idt_exception_25_wrapped, GDT_SELECTOR_CS_KERNEL, IDT_ENTRY_TYPE_INTERRUPT);
	idt_set_entry(26, (uint64_t) idt_exception_26_wrapped, GDT_SELECTOR_CS_KERNEL, IDT_ENTRY_TYPE_INTERRUPT);
	idt_set_entry(27, (uint64_t) idt_exception_27_wrapped, GDT_SELECTOR_CS_KERNEL, IDT_ENTRY_TYPE_INTERRUPT);
	idt_set_entry(28, (uint64_t) idt_exception_28_wrapped, GDT_SELECTOR_CS_KERNEL, IDT_ENTRY_TYPE_INTERRUPT);
	idt_set_entry(29, (uint64_t) idt_exception_29_wrapped, GDT_SELECTOR_CS_KERNEL, IDT_ENTRY_TYPE_INTERRUPT);
	idt_set_entry(30, (uint64_t) idt_exception_30_wrapped, GDT_SELECTOR_CS_KERNEL, IDT_ENTRY_TYPE_INTERRUPT);
	idt_set_entry(31, (uint64_t) idt_exception_31_wrapped, GDT_SELECTOR_CS_KERNEL, IDT_ENTRY_TYPE_INTERRUPT);
	
	idt_load(&idt_ptr);
	
	asm volatile("sti");
}

void idt_set_handler_keyboard(void (*handler)()) {
	idt_handler_keyboard_user = handler;
}

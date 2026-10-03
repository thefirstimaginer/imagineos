use core::arch::{asm, global_asm};
use core::cell::UnsafeCell;
use core::mem::size_of;

global_asm!(
    ".global dreamcore_exception_stub",
    "dreamcore_exception_stub:",
    "cli",
    "and rsp, -16",
    "call dreamcore_exception_handler",
    "1:",
    "hlt",
    "jmp 1b",
    ".global dreamcore_syscall_stub",
    "dreamcore_syscall_stub:",
    "push rax",
    "push rbx",
    "push rcx",
    "push rdx",
    "push rsi",
    "push rdi",
    "push rbp",
    "push r8",
    "push r9",
    "push r10",
    "push r11",
    "push r12",
    "push r13",
    "push r14",
    "push r15",
    "mov rdi, rsp",
    "and rsp, -16",
    "call dreamcore_syscall_dispatch",
    "mov rsp, rax",
    "pop r15",
    "pop r14",
    "pop r13",
    "pop r12",
    "pop r11",
    "pop r10",
    "pop r9",
    "pop r8",
    "pop rbp",
    "pop rdi",
    "pop rsi",
    "pop rdx",
    "pop rcx",
    "pop rbx",
    "pop rax",
    "iretq",
);

extern "C" {
    fn dreamcore_exception_stub();
    fn dreamcore_syscall_stub();
}

#[no_mangle]
extern "C" fn dreamcore_exception_handler() -> ! {
    serial_write(b"FATAL CPU EXCEPTION\r\n");
    loop {
        unsafe {
            asm!("hlt", options(nomem, nostack));
        }
    }
}

#[repr(C, packed)]
#[derive(Clone, Copy)]
struct IdtEntry {
    offset_low: u16,
    selector: u16,
    ist: u8,
    attributes: u8,
    offset_middle: u16,
    offset_high: u32,
    reserved: u32,
}

impl IdtEntry {
    const MISSING: Self = Self {
        offset_low: 0,
        selector: 0,
        ist: 0,
        attributes: 0,
        offset_middle: 0,
        offset_high: 0,
        reserved: 0,
    };

    fn interrupt_gate(handler: u64, selector: u16) -> Self {
        Self {
            offset_low: handler as u16,
            selector,
            ist: 0,
            attributes: 0x8e,
            offset_middle: (handler >> 16) as u16,
            offset_high: (handler >> 32) as u32,
            reserved: 0,
        }
    }
}

struct SharedIdt(UnsafeCell<[IdtEntry; 256]>);
unsafe impl Sync for SharedIdt {}
static IDT: SharedIdt = SharedIdt(UnsafeCell::new([IdtEntry::MISSING; 256]));

#[repr(C, packed)]
struct DescriptorTablePointer {
    limit: u16,
    base: u64,
}

pub fn init() {
    let idt = unsafe { &mut *IDT.0.get() };
    let selector: u16;
    unsafe {
        asm!("mov {0:x}, cs", out(reg) selector, options(nomem, nostack, preserves_flags));
    }
    let handler = dreamcore_exception_stub as *const () as usize as u64;
    for entry in idt.iter_mut().take(32) {
        *entry = IdtEntry::interrupt_gate(handler, selector);
    }
    let syscall_handler = dreamcore_syscall_stub as *const () as usize as u64;
    idt[0x80] = IdtEntry {
        attributes: 0xee,
        ..IdtEntry::interrupt_gate(syscall_handler, selector)
    };
    let pointer = DescriptorTablePointer {
        limit: (size_of::<[IdtEntry; 256]>() - 1) as u16,
        base: idt.as_ptr() as u64,
    };
    unsafe {
        asm!("lidt [{}]", in(reg) &pointer, options(readonly, nostack, preserves_flags));
    }
}

fn serial_write(bytes: &[u8]) {
    for &byte in bytes {
        unsafe {
            while in_port(0x3fd) & 0x20 == 0 {
                asm!("pause", options(nomem, nostack, preserves_flags));
            }
            asm!("out dx, al", in("dx") 0x3f8u16, in("al") byte, options(nomem, nostack, preserves_flags));
        }
    }
}

unsafe fn in_port(port: u16) -> u8 {
    let value: u8;
    asm!("in al, dx", in("dx") port, out("al") value, options(nomem, nostack, preserves_flags));
    value
}

#![no_std]
#![no_main]

extern crate alloc;

use alloc::vec::Vec;
use core::arch::asm;
use core::panic::PanicInfo;
use limine::request::{FramebufferRequest, HhdmRequest, MemoryMapRequest, ModuleRequest};
use limine::BaseRevision;

mod elf;
mod framebuffer;
mod gdt;
mod heap;
mod idt;
mod keyboard;
mod memory;
mod paging;
mod process;
mod ramfs;
mod syscall;

#[used]
#[link_section = ".requests"]
static BASE_REVISION: BaseRevision = BaseRevision::new();

#[used]
#[link_section = ".requests"]
static MEMORY_MAP_REQUEST: MemoryMapRequest = MemoryMapRequest::new();

#[used]
#[link_section = ".requests"]
static FRAMEBUFFER_REQUEST: FramebufferRequest = FramebufferRequest::new();

#[used]
#[link_section = ".requests"]
static HHDM_REQUEST: HhdmRequest = HhdmRequest::new();

#[used]
#[link_section = ".requests"]
static MODULE_REQUEST: ModuleRequest = ModuleRequest::new();

#[no_mangle]
pub extern "C" fn _start() -> ! {
    unsafe {
        asm!("cli", options(nomem, nostack, preserves_flags));
    }

    serial_init();
    mask_legacy_pic();
    gdt::init();
    idt::init();
    serial_write(b"ImagineOS Astrid Rust kernel\r\n");

    if !BASE_REVISION.is_supported() {
        serial_write(b"Limine protocol revision unsupported\r\n");
        halt();
    }

    let Some(memory_map) = MEMORY_MAP_REQUEST.get_response() else {
        serial_write(b"Limine memory map unavailable\r\n");
        halt();
    };

    let Some(hhdm) = HHDM_REQUEST.get_response() else {
        console_write("Limine HHDM unavailable; stopping safely\n");
        halt();
    };
    let hhdm_offset = hhdm.offset();
    let frame_count = memory::init(memory_map.entries(), hhdm_offset);
    let Some(boot_frame) = memory::allocate_frame() else {
        serial_write(b"No usable physical frames\r\n");
        halt();
    };
    unsafe {
        core::ptr::write_bytes(boot_frame.virtual_address, 0, 4096);
    }
    framebuffer::init(
        FRAMEBUFFER_REQUEST
            .get_response()
            .and_then(|response| response.framebuffers().next()),
    );
    console_write("ImagineOS Astrid Rust kernel\n");
    console_write("Frame allocator ready; usable frames: ");
    console_number(frame_count.saturating_sub(1) as u64);
    console_write("; reserved boot frame at physical 0x");
    console_hex(boot_frame.physical_address);
    console_write("\n");

    let archive = MODULE_REQUEST
        .get_response()
        .and_then(|response| {
            response
                .modules()
                .iter()
                .find(|module| module.path().to_bytes().ends_with(b"initrd.tar"))
        })
        .map(|module| unsafe {
            core::slice::from_raw_parts(module.addr(), module.size() as usize)
        });

    let Some(archive_bytes) = archive else {
        console_write("RAMFS initrd.tar missing; stopping safely\n");
        halt();
    };
    let archive = ramfs::Archive::new(archive_bytes);
    let Some(init_elf) = archive.find("init.elf") else {
        console_write("RAMFS has no /init.elf; stopping safely\n");
        halt();
    };
    let Some(getty_elf) = archive.find("getty.elf") else {
        console_write("RAMFS has no /getty.elf; stopping safely\n");
        halt();
    };
    let Some(shell_elf) = archive.find("shell.elf") else {
        console_write("RAMFS has no /shell.elf; stopping safely\n");
        halt();
    };

    if let Some(font) = archive.find("font.psf") {
        framebuffer::load_font(font);
    }
    console_write("RAMFS mounted; loading ring-3 ELF programs\n");
    let mut programs = Vec::new();
    if programs.try_reserve_exact(3).is_err() {
        console_write("Kernel heap exhausted while preparing init\n");
        halt();
    }
    programs.push((init_elf, 1));
    programs.push((getty_elf, 2));
    programs.push((shell_elf, 3));
    if process::init(&programs).is_err() {
        console_write("ELF loader failed; stopping safely\n");
        halt();
    }
    process::start()
}

pub(crate) fn console_write(text: &str) {
    serial_write(text.as_bytes());
    framebuffer::write_str(text);
}

pub(crate) fn console_write_bytes(bytes: &[u8]) {
    serial_write(bytes);
    if let Ok(text) = core::str::from_utf8(bytes) {
        framebuffer::write_str(text);
    }
}

fn console_number(mut value: u64) {
    let mut digits = [0u8; 20];
    let mut cursor = digits.len();
    if value == 0 {
        serial_write(b"0");
        framebuffer::write_char('0');
        return;
    }
    while value != 0 {
        cursor -= 1;
        digits[cursor] = b'0' + (value % 10) as u8;
        value /= 10;
    }
    serial_write(&digits[cursor..]);
    if let Ok(text) = core::str::from_utf8(&digits[cursor..]) {
        framebuffer::write_str(text);
    }
}

pub(crate) fn console_write_number(value: u64) {
    console_number(value);
}

fn console_hex(mut value: u64) {
    let mut digits = [0u8; 16];
    let mut cursor = digits.len();
    while value != 0 {
        cursor -= 1;
        let digit = (value & 0xf) as u8;
        digits[cursor] = if digit < 10 {
            b'0' + digit
        } else {
            b'a' + digit - 10
        };
        value >>= 4;
    }
    if cursor == digits.len() {
        cursor -= 1;
        digits[cursor] = b'0';
    }
    serial_write(&digits[cursor..]);
    if let Ok(text) = core::str::from_utf8(&digits[cursor..]) {
        framebuffer::write_str(text);
    }
}

fn serial_init() {
    unsafe {
        out(0x3f9, 0x00);
        out(0x3fb, 0x80);
        out(0x3f8, 0x03);
        out(0x3f9, 0x00);
        out(0x3fb, 0x03);
        out(0x3fa, 0xc7);
        out(0x3fc, 0x0b);
    }
}

fn mask_legacy_pic() {
    unsafe {
        out(0x21, 0xff);
        out(0xa1, 0xff);
    }
}

fn serial_write(bytes: &[u8]) {
    let mut previous_was_cr = false;
    for &byte in bytes {
        if byte == b'\n' && !previous_was_cr {
            serial_write_byte(b'\r');
        }
        serial_write_byte(byte);
        previous_was_cr = byte == b'\r';
    }
}

fn serial_write_byte(byte: u8) {
    unsafe {
        while in_port(0x3fd) & 0x20 == 0 {
            asm!("pause", options(nomem, nostack, preserves_flags));
        }
        out(0x3f8, byte);
    }
}

unsafe fn out(port: u16, value: u8) {
    asm!("out dx, al", in("dx") port, in("al") value, options(nomem, nostack, preserves_flags));
}

unsafe fn in_port(port: u16) -> u8 {
    let value: u8;
    asm!("in al, dx", in("dx") port, out("al") value, options(nomem, nostack, preserves_flags));
    value
}

fn halt() -> ! {
    loop {
        unsafe {
            asm!("hlt", options(nomem, nostack));
        }
    }
}

pub(crate) fn kernel_halt() -> ! {
    halt()
}

#[panic_handler]
fn panic(_info: &PanicInfo<'_>) -> ! {
    serial_write(b"KERNEL PANIC\r\n");
    halt()
}

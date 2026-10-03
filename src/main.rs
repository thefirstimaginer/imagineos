#![no_std]
#![no_main]

use core::arch::asm;
use core::panic::PanicInfo;
use limine::request::{FramebufferRequest, HhdmRequest, MemoryMapRequest, ModuleRequest};
use limine::BaseRevision;

mod framebuffer;
mod gdt;
mod idt;
mod keyboard;
mod memory;
mod ramfs;

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
    let Some(init_script) = archive.find("init") else {
        console_write("RAMFS has no /init; stopping safely\n");
        halt();
    };

    if let Some(font) = archive.find("font.psf") {
        framebuffer::load_font(font);
    }
    console_write("RAMFS mounted; /init found\n");
    run_script("init", init_script, &archive, 0)
}

fn run_script(path: &str, script: &[u8], archive: &ramfs::Archive<'_>, depth: usize) -> ! {
    if depth > 4 {
        console_write("init script recursion limit reached\n");
        halt();
    }
    if path == "shell" {
        console_write("Astrid getty ready. Type help for commands.\n> ");
        shell(archive);
    }
    let Some(script) = core::str::from_utf8(script).ok() else {
        console_write("init script is not valid UTF-8\n");
        halt();
    };
    for line in script.lines() {
        let line = line.trim();
        if line.is_empty() || line.starts_with('#') {
            continue;
        }
        if let Some(message) = line.strip_prefix("echo ") {
            console_write(message);
            console_write("\n");
        } else if let Some(program) = line.strip_prefix("exec /") {
            let Some(program_script) = archive.find(program) else {
                console_write("init requested a missing program\n");
                halt();
            };
            run_script(program, program_script, archive, depth + 1);
        } else {
            console_write("unsupported command in init script\n");
            halt();
        }
    }
    console_write("init exited without starting getty\n");
    halt()
}

fn shell(archive: &ramfs::Archive<'_>) -> ! {
    let mut keyboard = keyboard::Keyboard::new();
    let mut line = [0u8; 128];
    let mut length = 0;

    loop {
        let character = keyboard.poll_char().or_else(serial_read_char);
        let Some(character) = character else {
            core::hint::spin_loop();
            continue;
        };

        match character {
            '\r' | '\n' => {
                console_write("\n");
                let command = core::str::from_utf8(&line[..length]).unwrap_or("");
                execute_command(command, archive);
                line.fill(0);
                length = 0;
                console_write("> ");
            }
            '\u{8}' | '\u{7f}' => {
                if length > 0 {
                    length -= 1;
                    while length > 0 && line[length] & 0xc0 == 0x80 {
                        length -= 1;
                    }
                    console_write("\u{8} \u{8}");
                }
            }
            character if !character.is_control() => {
                let mut encoded = [0u8; 4];
                let bytes = character.encode_utf8(&mut encoded).as_bytes();
                if length + bytes.len() <= line.len() {
                    line[length..length + bytes.len()].copy_from_slice(bytes);
                    length += bytes.len();
                    framebuffer::write_char(character);
                    serial_write(bytes);
                }
            }
            _ => {}
        }
    }
}

fn execute_command(command: &str, archive: &ramfs::Archive<'_>) {
    match command {
        "" => {}
        "help" => console_write("help clear ls cat /init mem ps\n"),
        "clear" => framebuffer::clear(),
        "ls" => {
            console_write("/init\n/getty\n/shell\n");
            if archive.find("font.psf").is_some() {
                console_write("/font.psf\n");
            }
        }
        "cat /init" => console_write(
            core::str::from_utf8(archive.find("init").unwrap_or_default())
                .unwrap_or("invalid text\n"),
        ),
        "mem" => console_write("4 KiB frame allocator active\n"),
        "ps" => console_write("PID 0  kernel bootstrap (single foreground task)\n"),
        _ if command.starts_with("echo ") => {
            console_write(&command[5..]);
            console_write("\n");
        }
        _ => console_write("command not found\n"),
    }
}

fn console_write(text: &str) {
    serial_write(text.as_bytes());
    framebuffer::write_str(text);
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

fn serial_write(bytes: &[u8]) {
    for &byte in bytes {
        unsafe {
            while in_port(0x3fd) & 0x20 == 0 {
                asm!("pause", options(nomem, nostack, preserves_flags));
            }
            out(0x3f8, byte);
        }
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

fn serial_read_char() -> Option<char> {
    unsafe { (in_port(0x3fd) & 1 != 0).then(|| in_port(0x3f8) as char) }
}

fn halt() -> ! {
    loop {
        unsafe {
            asm!("hlt", options(nomem, nostack));
        }
    }
}

#[panic_handler]
fn panic(_info: &PanicInfo<'_>) -> ! {
    serial_write(b"KERNEL PANIC\r\n");
    halt()
}

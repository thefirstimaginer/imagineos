#![no_std]
#![no_main]

use core::arch::asm;
use core::panic::PanicInfo;

const SYS_WRITE: u64 = 1;
const SYS_READ: u64 = 2;
const SYS_YIELD: u64 = 3;
const SYS_EXIT: u64 = 4;
const SYS_CLEAR: u64 = 6;

#[no_mangle]
extern "C" fn _start() -> ! {
    let mut line = [0u8; 128];
    let mut length = 0usize;
    write("Astrid shell (ring 3). Type help.\n> ");

    loop {
        let codepoint = syscall0(SYS_READ) as u32;
        let Some(character) = char::from_u32(codepoint) else { continue };
        match character {
            '\r' | '\n' => {
                write("\n");
                run_command(&line[..length]);
                line.fill(0);
                length = 0;
                write("> ");
            }
            '\u{8}' | '\u{7f}' => {
                if length > 0 {
                    length -= 1;
                    while length > 0 && line[length] & 0xc0 == 0x80 { length -= 1; }
                    write("\u{8} \u{8}");
                }
            }
            value if !value.is_control() => {
                let mut encoded = [0u8; 4];
                let bytes = value.encode_utf8(&mut encoded).as_bytes();
                if length + bytes.len() <= line.len() {
                    line[length..length + bytes.len()].copy_from_slice(bytes);
                    length += bytes.len();
                    unsafe { syscall3(SYS_WRITE, bytes.as_ptr() as u64, bytes.len() as u64, 0); }
                }
            }
            _ => {}
        }
        syscall0(SYS_YIELD);
    }
}

fn run_command(line: &[u8]) {
    match line {
        b"" => {}
        b"help" => write("help clear pid echo <text> exit\n"),
        b"clear" => { syscall0(SYS_CLEAR); }
        b"pid" => {
            let mut digits = [0u8; 20];
            let mut position = digits.len();
            let mut value = syscall0(5);
            if value == 0 { write("pid 0\n"); return; }
            while value != 0 {
                position -= 1;
                digits[position] = b'0' + (value % 10) as u8;
                value /= 10;
            }
            write("pid ");
            write_bytes(&digits[position..]);
            write("\n");
        }
        b"exit" => { syscall0(SYS_EXIT); }
        _ if line.starts_with(b"echo ") => {
            write_bytes(&line[5..]);
            write("\n");
        }
        _ => write("command not found\n"),
    }
}

fn write(text: &str) {
    write_bytes(text.as_bytes());
}

fn write_bytes(bytes: &[u8]) {
    unsafe { syscall3(SYS_WRITE, bytes.as_ptr() as u64, bytes.len() as u64, 0); }
}

fn syscall0(number: u64) -> u64 {
    unsafe { syscall3(number, 0, 0, 0) }
}

unsafe fn syscall3(number: u64, first: u64, second: u64, third: u64) -> u64 {
    let result: u64;
    asm!(
        "int 0x80",
        inout("rax") number => result,
        in("rdi") first,
        in("rsi") second,
        in("rdx") third,
    );
    result
}

#[panic_handler]
fn panic(_info: &PanicInfo<'_>) -> ! {
    loop { core::hint::spin_loop(); }
}

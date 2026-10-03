use core::arch::asm;
use core::cell::UnsafeCell;

use crate::keyboard::Keyboard;
use crate::{framebuffer, process};

pub const SYS_WRITE: u64 = 1;
pub const SYS_READ: u64 = 2;
pub const SYS_YIELD: u64 = 3;
pub const SYS_EXIT: u64 = 4;
pub const SYS_GETPID: u64 = 5;
pub const SYS_CLEAR: u64 = 6;

struct SharedKeyboard(UnsafeCell<Keyboard>);
unsafe impl Sync for SharedKeyboard {}
static KEYBOARD: SharedKeyboard = SharedKeyboard(UnsafeCell::new(Keyboard::new()));

#[repr(C)]
pub struct TrapFrame {
    pub r15: u64,
    pub r14: u64,
    pub r13: u64,
    pub r12: u64,
    pub r11: u64,
    pub r10: u64,
    pub r9: u64,
    pub r8: u64,
    pub rbp: u64,
    pub rdi: u64,
    pub rsi: u64,
    pub rdx: u64,
    pub rcx: u64,
    pub rbx: u64,
    pub rax: u64,
    pub rip: u64,
    pub cs: u64,
    pub rflags: u64,
    pub rsp: u64,
    pub ss: u64,
}

#[no_mangle]
extern "C" fn dreamcore_syscall_dispatch(frame: *mut TrapFrame) -> *mut TrapFrame {
    let Some(frame_ref) = (unsafe { frame.as_mut() }) else {
        return frame;
    };
    match frame_ref.rax {
        SYS_YIELD => {
            frame_ref.rax = 0;
            process::yield_current(frame)
        }
        SYS_EXIT => process::exit_current(frame),
        SYS_GETPID => {
            frame_ref.rax = process::current_pid() as u64;
            frame
        }
        SYS_WRITE => {
            let length = (frame_ref.rsi as usize).min(512);
            let mut buffer = [0u8; 512];
            if process::copy_from_current_user(frame_ref.rdi, &mut buffer[..length]) {
                crate::console_write_bytes(&buffer[..length]);
                frame_ref.rax = length as u64;
            } else {
                frame_ref.rax = (-14i64) as u64;
            }
            frame
        }
        SYS_READ => {
            frame_ref.rax = read_character() as u64;
            frame
        }
        SYS_CLEAR => {
            framebuffer::clear();
            frame_ref.rax = 0;
            frame
        }
        _ => {
            frame_ref.rax = u64::MAX;
            frame
        }
    }
}

fn read_character() -> u32 {
    loop {
        if let Some(character) = unsafe { (&mut *KEYBOARD.0.get()).poll_char() } {
            return character as u32;
        }
        if let Some(character) = serial_read_char() {
            return character as u32;
        }
        core::hint::spin_loop();
    }
}

fn serial_read_char() -> Option<char> {
    unsafe {
        if in_port(0x3fd) & 1 != 0 {
            Some(in_port(0x3f8) as char)
        } else {
            None
        }
    }
}

unsafe fn in_port(port: u16) -> u8 {
    let value: u8;
    asm!("in al, dx", in("dx") port, out("al") value, options(nomem, nostack, preserves_flags));
    value
}

#![no_std]
#![no_main]

use core::arch::asm;
use core::panic::PanicInfo;

const SYS_WRITE: u64 = 1;
const SYS_EXIT: u64 = 4;
const SYS_EXEC: u64 = 7;

#[repr(C)]
struct UserArg {
    address: u64,
    length: u64,
}

#[no_mangle]
extern "C" fn _start(
    _argc: usize,
    _argv: *const *const u8,
    _envc: usize,
    _envp: *const *const u8,
) -> ! {
    write("Astrid init: running as ELF in ring 3\n");
    exec("/bin/getty");
    syscall0(SYS_EXIT);
    loop {
        core::hint::spin_loop();
    }
}

fn write(text: &str) {
    unsafe {
        syscall3(SYS_WRITE, text.as_ptr() as u64, text.len() as u64, 0);
    }
}

fn syscall0(number: u64) -> u64 {
    unsafe { syscall3(number, 0, 0, 0) }
}

fn exec(path: &str) -> u64 {
    let argument = UserArg {
        address: path.as_ptr() as u64,
        length: path.len() as u64,
    };
    unsafe {
        syscall6(
            SYS_EXEC,
            path.as_ptr() as u64,
            path.len() as u64,
            &argument as *const UserArg as u64,
            1,
            0,
            0,
        )
    }
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

unsafe fn syscall6(
    number: u64,
    first: u64,
    second: u64,
    third: u64,
    fourth: u64,
    fifth: u64,
    sixth: u64,
) -> u64 {
    let result: u64;
    asm!(
        "int 0x80",
        inout("rax") number => result,
        in("rdi") first,
        in("rsi") second,
        in("rdx") third,
        in("r10") fourth,
        in("r8") fifth,
        in("r9") sixth,
    );
    result
}

#[panic_handler]
fn panic(_info: &PanicInfo<'_>) -> ! {
    loop {
        core::hint::spin_loop();
    }
}

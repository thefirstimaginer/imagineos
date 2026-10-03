#![no_std]
#![no_main]

use core::arch::asm;
use core::panic::PanicInfo;

const SYS_WRITE: u64 = 1;
const SYS_YIELD: u64 = 3;
const SYS_EXIT: u64 = 4;

#[no_mangle]
extern "C" fn _start() -> ! {
    write("Astrid getty: console ready\n");
    syscall0(SYS_YIELD);
    syscall0(SYS_EXIT);
    loop { core::hint::spin_loop(); }
}

fn write(text: &str) {
    unsafe { syscall3(SYS_WRITE, text.as_ptr() as u64, text.len() as u64, 0); }
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

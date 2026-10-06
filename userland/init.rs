#![no_std]
#![no_main]

use core::panic::PanicInfo;

#[no_mangle]
extern "C" fn _start(
    _argc: usize,
    _argv: *const *const u8,
    _envc: usize,
    _envp: *const *const u8,
) -> ! {
    if imagineos::console::write_all(b"Astrid init: PID 1 running in ring 3\n").is_err() {
        imagineos::process::exit(1);
    }
    if imagineos::process::exec("/sbin/getty", &["/sbin/getty"], &[]).is_err() {
        imagineos::process::exit(1);
    }
    imagineos::process::exit(0)
}

#[panic_handler]
fn panic(_info: &PanicInfo<'_>) -> ! {
    imagineos::process::exit(127)
}

#![no_std]
#![no_main]

#[allow(dead_code)]
mod common;

#[no_mangle]
extern "C" fn _start(
    _argc: usize,
    _argv: *const *const u8,
    _envc: usize,
    _envp: *const *const u8,
) -> ! {
    common::write(b"Hello from ImagineOS!\n");
    common::exit(0)
}

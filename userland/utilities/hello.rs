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
    if imagineos::syscall::abi_version() != Ok(imagineos::abi::ABI_VERSION) {
        imagineos::process::exit(2);
    }
    if imagineos::console::write_all(b"Hello from ImagineOS!\n").is_err() {
        imagineos::process::exit(1);
    }
    imagineos::process::exit(0)
}

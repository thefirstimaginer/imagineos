//Vi text editor for ImagineOS Astrid
#![no_std]
#![no_main]

#[allow(dead_code)]
mod common;

#[no_mangle]
extern "C" fn _start(
    argc: usize,
    argv: *const *const u8,
    envc: usize,
    envp: *const *const u8,
) -> ! {
    if argc < 2 {
        common::write(b"usage: vi FILE\n");
        common::exit(2);
    }
    let mut path = [0u8; 256];
    let Some(length) = common::resolve_path(envp, envc, argv, &mut path) else {
        common::write(b"vi: path too long\n");
        common::exit(2);
    };
    if common::edit_file(&path[..length]).is_err() {
        common::write(b"vi: cannot edit file\n");
        common::exit(2);
    }
    common::exit(0)
}
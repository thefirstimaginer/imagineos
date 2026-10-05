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
    let requested = if argc > 1 {
        unsafe { common::argument(argv, 1).unwrap_or(b"/") }
    } else {
        unsafe { common::environment_value(envp, envc, b"PWD").unwrap_or(b"/") }
    };
    let mut path = [0u8; 256];
    let Some(path_length) = common::resolve_path(envp, envc, requested, &mut path) else {
        common::write(b"ls: path too long\n");
        common::exit(2);
    };
    let mut entries = [0u8; 2048];
    let result = common::list_directory(&path[..path_length], &mut entries);
    if result < 0 {
        common::write(b"ls: directory not found\n");
        common::exit(2);
    }
    common::write(&entries[..result as usize]);
    common::exit(0)
}

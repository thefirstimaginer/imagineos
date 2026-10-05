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
        common::write(b"usage: cat FILE...\n");
        common::exit(2);
    }
    let mut buffer = [0u8; 4096];
    for index in 1..argc {
        let Some(path) = (unsafe { common::argument(argv, index) }) else {
            continue;
        };
        let mut resolved = [0u8; 256];
        let Some(path_length) = common::resolve_path(envp, envc, path, &mut resolved) else {
            common::write(b"cat: path too long\n");
            continue;
        };
        let result = common::read_file(&resolved[..path_length], &mut buffer);
        if result < 0 {
            common::write(b"cat: cannot read file\n");
            continue;
        }
        common::write(&buffer[..result as usize]);
    }
    common::exit(0)
}

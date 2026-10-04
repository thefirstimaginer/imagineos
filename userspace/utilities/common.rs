use core::arch::asm;
use core::panic::PanicInfo;

pub const SYS_WRITE: u64 = 1;
pub const SYS_EXIT: u64 = 4;
pub const SYS_READ_FILE: u64 = 10;
pub const SYS_READDIR: u64 = 11;

pub unsafe fn argument(argv: *const *const u8, index: usize) -> Option<&'static [u8]> {
    let pointer = *argv.add(index);
    if pointer.is_null() {
        return None;
    }
    let mut length = 0usize;
    while length < 256 && *pointer.add(length) != 0 {
        length += 1;
    }
    Some(core::slice::from_raw_parts(pointer, length))
}

pub unsafe fn environment_value(
    envp: *const *const u8,
    count: usize,
    name: &[u8],
) -> Option<&'static [u8]> {
    for index in 0..count {
        let entry = argument(envp, index)?;
        if entry.starts_with(name) && entry.get(name.len()) == Some(&b'=') {
            return Some(&entry[name.len() + 1..]);
        }
    }
    None
}

pub fn resolve_path(
    envp: *const *const u8,
    envc: usize,
    path: &[u8],
    output: &mut [u8],
) -> Option<usize> {
    if path.starts_with(b"/") {
        if path.len() > output.len() {
            return None;
        }
        output[..path.len()].copy_from_slice(path);
        return Some(path.len());
    }
    let cwd = unsafe { environment_value(envp, envc, b"PWD") }.unwrap_or(b"/");
    let mut length = cwd.len();
    if length + 1 + path.len() > output.len() {
        return None;
    }
    output[..length].copy_from_slice(cwd);
    if length == 0 || output[length - 1] != b'/' {
        output[length] = b'/';
        length += 1;
    }
    output[length..length + path.len()].copy_from_slice(path);
    Some(length + path.len())
}

pub fn write(bytes: &[u8]) {
    unsafe {
        syscall3(SYS_WRITE, bytes.as_ptr() as u64, bytes.len() as u64, 0);
    }
}

pub fn read_file(path: &[u8], output: &mut [u8]) -> i64 {
    unsafe {
        syscall4(
            SYS_READ_FILE,
            path.as_ptr() as u64,
            path.len() as u64,
            output.as_mut_ptr() as u64,
            output.len() as u64,
        ) as i64
    }
}

pub fn list_directory(path: &[u8], output: &mut [u8]) -> i64 {
    unsafe {
        syscall4(
            SYS_READDIR,
            path.as_ptr() as u64,
            path.len() as u64,
            output.as_mut_ptr() as u64,
            output.len() as u64,
        ) as i64
    }
}

pub fn exit(status: u64) -> ! {
    unsafe {
        syscall3(SYS_EXIT, status, 0, 0);
    }
    loop {
        core::hint::spin_loop();
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

unsafe fn syscall4(number: u64, first: u64, second: u64, third: u64, fourth: u64) -> u64 {
    let result: u64;
    asm!(
        "int 0x80",
        inout("rax") number => result,
        in("rdi") first,
        in("rsi") second,
        in("rdx") third,
        in("r10") fourth,
    );
    result
}

#[panic_handler]
fn panic(_info: &PanicInfo<'_>) -> ! {
    exit(127)
}

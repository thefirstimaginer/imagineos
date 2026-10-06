use crate::{args, console, fs, process};

pub unsafe fn argument(argv: *const *const u8, index: usize) -> Option<&'static [u8]> {
    args::argument(argv, index.saturating_add(1), index)
}

pub unsafe fn environment_value(
    envp: *const *const u8,
    count: usize,
    name: &[u8],
) -> Option<&'static [u8]> {
    args::environment_value(envp, count, name)
}

pub fn resolve_path(
    envp: *const *const u8,
    envc: usize,
    path: &[u8],
    output: &mut [u8],
) -> Option<usize> {
    let cwd = unsafe { environment_value(envp, envc, b"PWD") };
    args::resolve_path(cwd, path, output)
}

pub fn write(bytes: &[u8]) {
    if console::write_all(bytes).is_err() {
        process::exit(1);
    }
}

pub fn read_char() -> char {
    match console::read_char() {
        Ok(character) => character,
        Err(_) => process::exit(1),
    }
}

pub fn clear() {
    if console::clear().is_err() {
        process::exit(1);
    }
}

pub fn read_file(path: &[u8], output: &mut [u8]) -> i64 {
    let Ok(path) = core::str::from_utf8(path) else {
        return -22;
    };
    fs::read_file(path, output).map_or_else(|error| -(error.code() as i64), |count| count as i64)
}

pub fn write_file(path: &[u8], contents: &[u8]) -> i64 {
    let Ok(path) = core::str::from_utf8(path) else {
        return -22;
    };
    fs::write_file(path, contents).map_or_else(|error| -(error.code() as i64), |count| count as i64)
}

pub fn list_directory(path: &[u8], output: &mut [u8]) -> i64 {
    let Ok(path) = core::str::from_utf8(path) else {
        return -22;
    };
    fs::list_directory(path, output)
        .map_or_else(|error| -(error.code() as i64), |count| count as i64)
}

pub fn mkdir(path: &[u8], parents: bool) -> i64 {
    let Ok(path) = core::str::from_utf8(path) else {
        return -22;
    };
    fs::mkdir(path, parents).map_or_else(|error| -(error.code() as i64), |_| 0)
}

pub fn touch(path: &[u8]) -> i64 {
    let Ok(path) = core::str::from_utf8(path) else {
        return -22;
    };
    fs::touch(path).map_or_else(|error| -(error.code() as i64), |_| 0)
}

pub fn remove(path: &[u8], recursive: bool) -> i64 {
    let Ok(path) = core::str::from_utf8(path) else {
        return -22;
    };
    fs::remove(path, recursive).map_or_else(|error| -(error.code() as i64), |_| 0)
}

pub fn exit(status: u64) -> ! {
    process::exit(status as i32)
}

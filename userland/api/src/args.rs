/// Reads one NUL-terminated argument from the process startup vector.
///
/// # Safety
/// `argv` must point to an array of at least `count` readable pointers, and
/// every non-null entry must point to readable memory containing a NUL byte
/// within 256 bytes.
pub unsafe fn argument<'a>(argv: *const *const u8, count: usize, index: usize) -> Option<&'a [u8]> {
    if index >= count || argv.is_null() {
        return None;
    }
    let pointer = *argv.add(index);
    c_string(pointer)
}

/// Finds a `NAME=value` item in the process environment vector.
///
/// # Safety
/// `envp` must point to an array of at least `count` readable pointers, and
/// every non-null entry must point to readable memory containing a NUL byte
/// within 256 bytes.
pub unsafe fn environment_value<'a>(
    envp: *const *const u8,
    count: usize,
    name: &[u8],
) -> Option<&'a [u8]> {
    for index in 0..count {
        let Some(entry) = argument(envp, count, index) else {
            continue;
        };
        if entry.starts_with(name) && entry.get(name.len()) == Some(&b'=') {
            return Some(&entry[name.len() + 1..]);
        }
    }
    None
}

pub fn resolve_path(cwd: Option<&[u8]>, path: &[u8], output: &mut [u8]) -> Option<usize> {
    if path.starts_with(b"/") {
        if path.len() > output.len() {
            return None;
        }
        output[..path.len()].copy_from_slice(path);
        return Some(path.len());
    }

    let cwd = cwd.filter(|value| !value.is_empty()).unwrap_or(b"/");
    let separator = usize::from(!cwd.is_empty() && cwd.last() != Some(&b'/'));
    let length = cwd.len().checked_add(separator)?.checked_add(path.len())?;
    if length > output.len() {
        return None;
    }
    output[..cwd.len()].copy_from_slice(cwd);
    let mut cursor = cwd.len();
    if separator != 0 {
        output[cursor] = b'/';
        cursor += 1;
    }
    output[cursor..length].copy_from_slice(path);
    Some(length)
}

unsafe fn c_string<'a>(pointer: *const u8) -> Option<&'a [u8]> {
    if pointer.is_null() {
        return None;
    }
    let mut length = 0usize;
    while length < 256 && *pointer.add(length) != 0 {
        length += 1;
    }
    Some(core::slice::from_raw_parts(pointer, length))
}

#[cfg(test)]
mod tests {
    use super::resolve_path;

    #[test]
    fn resolves_absolute_and_relative_paths() {
        let mut output = [0; 32];
        assert_eq!(resolve_path(Some(b"/home"), b"file", &mut output), Some(10));
        assert_eq!(&output[..10], b"/home/file");
        assert_eq!(resolve_path(None, b"/bin/app", &mut output), Some(8));
        assert_eq!(&output[..8], b"/bin/app");
    }
}

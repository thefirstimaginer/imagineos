use core::cell::UnsafeCell;

const BLOCK_SIZE: usize = 512;

pub struct Archive<'a> {
    bytes: &'a [u8],
}

impl<'a> Archive<'a> {
    pub const fn new(bytes: &'a [u8]) -> Self {
        Self { bytes }
    }

    pub fn find(&self, wanted: &str) -> Option<&'a [u8]> {
        let wanted = wanted.trim_start_matches('/');
        let mut offset = 0usize;

        while offset.checked_add(BLOCK_SIZE)? <= self.bytes.len() {
            let header = &self.bytes[offset..offset + BLOCK_SIZE];
            if header.iter().all(|byte| *byte == 0) {
                return None;
            }

            let name = field(&header[..100]);
            let prefix = field(&header[345..500]);
            let path_matches = if prefix.is_empty() {
                name == wanted.as_bytes()
            } else {
                wanted
                    .strip_prefix(core::str::from_utf8(prefix).ok()?)
                    .is_some_and(|suffix| {
                        suffix
                            .strip_prefix('/')
                            .is_some_and(|rest| rest.as_bytes() == name)
                    })
            };
            let size = parse_octal(&header[124..136])?;
            let data_start = offset.checked_add(BLOCK_SIZE)?;
            let data_end = data_start.checked_add(size)?;
            if data_end > self.bytes.len() {
                return None;
            }
            if path_matches && (header[156] == 0 || header[156] == b'0') {
                return Some(&self.bytes[data_start..data_end]);
            }

            let padded_size = size.checked_add(BLOCK_SIZE - 1)? & !(BLOCK_SIZE - 1);
            offset = data_start.checked_add(padded_size)?;
        }
        None
    }

    pub fn find_font(&self) -> Option<&'a [u8]> {
        let mut offset = 0usize;
        while offset.checked_add(BLOCK_SIZE)? <= self.bytes.len() {
            let header = &self.bytes[offset..offset + BLOCK_SIZE];
            if header.iter().all(|byte| *byte == 0) {
                return None;
            }
            let name = field(&header[..100]);
            let prefix = field(&header[345..500]);
            let mut path = [0u8; 256];
            let path_len = if prefix.is_empty() {
                if name.len() > path.len() {
                    return None;
                }
                path[..name.len()].copy_from_slice(name);
                name.len()
            } else {
                let required = prefix.len().checked_add(1)?.checked_add(name.len())?;
                if required > path.len() {
                    return None;
                }
                path[..prefix.len()].copy_from_slice(prefix);
                path[prefix.len()] = b'/';
                path[prefix.len() + 1..required].copy_from_slice(name);
                required
            };
            let path = &path[..path_len];
            let in_font_directory = path.starts_with(b"system/fonts/");
            let supported_font = path.ends_with(b".psf") || path.ends_with(b".psf2");
            let size = parse_octal(&header[124..136])?;
            let data_start = offset.checked_add(BLOCK_SIZE)?;
            let data_end = data_start.checked_add(size)?;
            if data_end > self.bytes.len() {
                return None;
            }
            if in_font_directory && supported_font && (header[156] == 0 || header[156] == b'0') {
                return Some(&self.bytes[data_start..data_end]);
            }
            let padded_size = size.checked_add(BLOCK_SIZE - 1)? & !(BLOCK_SIZE - 1);
            offset = data_start.checked_add(padded_size)?;
        }
        None
    }

    pub fn is_directory(&self, wanted: &str) -> bool {
        let wanted = wanted.trim_start_matches('/');
        if wanted.is_empty() {
            return true;
        }
        let mut offset = 0usize;
        while offset
            .checked_add(BLOCK_SIZE)
            .is_some_and(|end| end <= self.bytes.len())
        {
            let header = &self.bytes[offset..offset + BLOCK_SIZE];
            if header.iter().all(|byte| *byte == 0) {
                return false;
            }
            let name = field(&header[..100]);
            let prefix = field(&header[345..500]);
            let mut path = [0u8; 256];
            let path_len = if prefix.is_empty() {
                if name.len() > path.len() {
                    return false;
                }
                path[..name.len()].copy_from_slice(name);
                name.len()
            } else {
                let Some(required) = prefix
                    .len()
                    .checked_add(1)
                    .and_then(|n| n.checked_add(name.len()))
                else {
                    return false;
                };
                if required > path.len() {
                    return false;
                }
                path[..prefix.len()].copy_from_slice(prefix);
                path[prefix.len()] = b'/';
                path[prefix.len() + 1..required].copy_from_slice(name);
                required
            };
            let current = &path[..path_len];
            if (current == wanted.as_bytes() && header[156] == b'5')
                || current
                    .strip_prefix(wanted.as_bytes())
                    .is_some_and(|rest| rest.starts_with(b"/"))
            {
                return true;
            }
            let Some(size) = parse_octal(&header[124..136]) else {
                return false;
            };
            let Some(data_start) = offset.checked_add(BLOCK_SIZE) else {
                return false;
            };
            let Some(padded_size) = size
                .checked_add(BLOCK_SIZE - 1)
                .map(|size| size & !(BLOCK_SIZE - 1))
            else {
                return false;
            };
            let Some(next) = data_start.checked_add(padded_size) else {
                return false;
            };
            if next > self.bytes.len() {
                return false;
            }
            offset = next;
        }
        false
    }

    pub fn is_file(&self, wanted: &str) -> bool {
        self.find(wanted).is_some()
    }

    pub fn list_directory(&self, directory: &str, output: &mut [u8]) -> Option<usize> {
        let wanted = directory.trim_matches('/');
        let mut offset = 0usize;
        let mut written = 0usize;
        while offset.checked_add(BLOCK_SIZE)? <= self.bytes.len() {
            let header = &self.bytes[offset..offset + BLOCK_SIZE];
            if header.iter().all(|byte| *byte == 0) {
                break;
            }
            let name = field(&header[..100]);
            let prefix = field(&header[345..500]);
            let mut full_path = [0u8; 256];
            let path_length = if prefix.is_empty() {
                if name.len() > full_path.len() {
                    return None;
                }
                full_path[..name.len()].copy_from_slice(name);
                name.len()
            } else {
                let required = prefix.len().checked_add(1)?.checked_add(name.len())?;
                if required > full_path.len() {
                    return None;
                }
                full_path[..prefix.len()].copy_from_slice(prefix);
                full_path[prefix.len()] = b'/';
                full_path[prefix.len() + 1..required].copy_from_slice(name);
                required
            };
            let full_path = &full_path[..path_length];
            let relative = if wanted.is_empty() {
                Some(full_path)
            } else {
                full_path
                    .strip_prefix(wanted.as_bytes())
                    .and_then(|suffix| suffix.strip_prefix(b"/"))
            };
            if let Some(relative) = relative {
                if let Some(component) = relative.split(|byte| *byte == b'/').next() {
                    if !component.is_empty() {
                        let mut already_listed = false;
                        for existing in output[..written].split(|byte| *byte == b'\n') {
                            if existing == component {
                                already_listed = true;
                                break;
                            }
                        }
                        if !already_listed {
                            let required = written.checked_add(component.len())?.checked_add(1)?;
                            if required > output.len() {
                                return None;
                            }
                            output[written..written + component.len()].copy_from_slice(component);
                            output[required - 1] = b'\n';
                            written = required;
                        }
                    }
                }
            }
            let size = parse_octal(&header[124..136])?;
            let data_start = offset.checked_add(BLOCK_SIZE)?;
            let padded_size = size.checked_add(BLOCK_SIZE - 1)? & !(BLOCK_SIZE - 1);
            offset = data_start.checked_add(padded_size)?;
        }
        Some(written)
    }
}

struct SharedArchive(UnsafeCell<Option<Archive<'static>>>);
unsafe impl Sync for SharedArchive {}
static MOUNTED_RAMFS: SharedArchive = SharedArchive(UnsafeCell::new(None));

pub fn mount(bytes: &'static [u8]) {
    unsafe {
        *MOUNTED_RAMFS.0.get() = Some(Archive::new(bytes));
    }
}

pub fn read(path: &str) -> Option<&'static [u8]> {
    let archive = unsafe { &*MOUNTED_RAMFS.0.get() };
    archive.as_ref()?.find(path)
}

pub fn find_font() -> Option<&'static [u8]> {
    let archive = unsafe { &*MOUNTED_RAMFS.0.get() };
    archive.as_ref()?.find_font()
}

pub fn is_directory(path: &str) -> bool {
    let archive = unsafe { &*MOUNTED_RAMFS.0.get() };
    archive
        .as_ref()
        .is_some_and(|archive| archive.is_directory(path))
}

pub fn is_file(path: &str) -> bool {
    let archive = unsafe { &*MOUNTED_RAMFS.0.get() };
    archive
        .as_ref()
        .is_some_and(|archive| archive.is_file(path))
}

pub fn list_directory(path: &str, output: &mut [u8]) -> Option<usize> {
    let archive = unsafe { &*MOUNTED_RAMFS.0.get() };
    archive.as_ref()?.list_directory(path, output)
}

fn field(bytes: &[u8]) -> &[u8] {
    let end = bytes
        .iter()
        .position(|byte| *byte == 0)
        .unwrap_or(bytes.len());
    &bytes[..end]
}

fn parse_octal(bytes: &[u8]) -> Option<usize> {
    let mut value = 0usize;
    let mut found_digit = false;
    for byte in bytes
        .iter()
        .copied()
        .skip_while(|byte| *byte == b' ' || *byte == 0)
    {
        if byte == 0 || byte == b' ' {
            break;
        }
        if !(b'0'..=b'7').contains(&byte) {
            return None;
        }
        found_digit = true;
        value = value.checked_mul(8)?.checked_add((byte - b'0') as usize)?;
    }
    found_digit.then_some(value)
}

#[cfg(test)]
mod tests {
    use super::Archive;

    fn archive_with_file(prefix: &[u8], name: &[u8], contents: &[u8]) -> Vec<u8> {
        let size = (contents.len() + 511) & !511;
        let mut bytes = vec![0u8; 512 + size + 1024];
        bytes[..name.len()].copy_from_slice(name);
        bytes[124..135].copy_from_slice(format!("{:011o}", contents.len()).as_bytes());
        bytes[135] = 0;
        bytes[156] = b'0';
        bytes[257..262].copy_from_slice(b"ustar");
        bytes[345..345 + prefix.len()].copy_from_slice(prefix);
        bytes[512..512 + contents.len()].copy_from_slice(contents);
        bytes
    }

    #[test]
    fn finds_root_file_and_accepts_leading_slash() {
        let bytes = archive_with_file(b"", b"init", b"abc");
        let archive = Archive::new(&bytes);
        assert_eq!(archive.find("init"), Some(&b"abc"[..]));
        assert_eq!(archive.find("/init"), Some(&b"abc"[..]));
    }

    #[test]
    fn rejects_missing_and_truncated_entries() {
        let mut bytes = archive_with_file(b"", b"init", b"abc");
        let archive = Archive::new(&bytes);
        assert_eq!(archive.find("missing"), None);
        bytes[124..136].copy_from_slice(b"77777777777\0");
        assert_eq!(Archive::new(&bytes).find("init"), None);
    }

    #[test]
    fn finds_bin_program_and_nested_psf_font() {
        let program = archive_with_file(b"bin", b"init", b"ELF");
        assert_eq!(Archive::new(&program).find("/bin/init"), Some(&b"ELF"[..]));
        assert!(Archive::new(&program).is_directory("bin"));

        let font = archive_with_file(b"system/fonts", b"default8x9.psf", b"PSF");
        assert_eq!(Archive::new(&font).find_font(), Some(&b"PSF"[..]));
        assert!(Archive::new(&font).is_directory("system/fonts"));
        assert!(!Archive::new(&font).is_directory("sbin"));

        let program = archive_with_file(b"bin", b"init", b"ELF");
        let mut entries = [0u8; 32];
        let length = Archive::new(&program)
            .list_directory("/bin", &mut entries)
            .unwrap();
        assert_eq!(&entries[..length], b"init\n");
    }
}

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

    fn archive_with_file(contents: &[u8]) -> [u8; 1024] {
        let mut bytes = [0u8; 1024];
        bytes[..8].copy_from_slice(b"init\0\0\0\0");
        bytes[124..136].copy_from_slice(b"00000000003\0");
        bytes[156] = b'0';
        bytes[257..262].copy_from_slice(b"ustar");
        bytes[512..512 + contents.len()].copy_from_slice(contents);
        bytes
    }

    #[test]
    fn finds_root_file_and_accepts_leading_slash() {
        let bytes = archive_with_file(b"abc");
        let archive = Archive::new(&bytes);
        assert_eq!(archive.find("init"), Some(&b"abc"[..]));
        assert_eq!(archive.find("/init"), Some(&b"abc"[..]));
    }

    #[test]
    fn rejects_missing_and_truncated_entries() {
        let mut bytes = archive_with_file(b"abc");
        let archive = Archive::new(&bytes);
        assert_eq!(archive.find("missing"), None);
        bytes[124..136].copy_from_slice(b"77777777777\0");
        assert_eq!(Archive::new(&bytes).find("init"), None);
    }
}

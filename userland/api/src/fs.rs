use crate::{checked, path_bytes, syscall, Error, Result};

pub fn is_dir(path: &str) -> Result<bool> {
    let path = path_bytes(path, crate::abi::MAX_PATH_QUERY)?;
    Ok(checked(syscall::invoke(
        crate::abi::SYS_ISDIR,
        [path.as_ptr() as u64, path.len() as u64, 0, 0, 0, 0],
    ))? != 0)
}

pub fn is_file(path: &str) -> Result<bool> {
    let path = path_bytes(path, crate::abi::MAX_PATH_QUERY)?;
    Ok(checked(syscall::invoke(
        crate::abi::SYS_ISFILE,
        [path.as_ptr() as u64, path.len() as u64, 0, 0, 0, 0],
    ))? != 0)
}

pub fn read_file(path: &str, output: &mut [u8]) -> Result<usize> {
    let path = path_bytes(path, crate::abi::MAX_PATH_QUERY)?;
    if output.len() > crate::abi::MAX_READ_BUFFER {
        return Err(Error::INVALID_ARGUMENT);
    }
    let result = checked(syscall::invoke(
        crate::abi::SYS_READ_FILE,
        [
            path.as_ptr() as u64,
            path.len() as u64,
            output.as_mut_ptr() as u64,
            output.len() as u64,
            0,
            0,
        ],
    ))?;
    usize::try_from(result).map_err(|_| Error::INVALID_ARGUMENT)
}

pub fn list_directory(path: &str, output: &mut [u8]) -> Result<usize> {
    let path = path_bytes(path, crate::abi::MAX_PATH_QUERY)?;
    if output.len() > crate::abi::MAX_READ_BUFFER {
        return Err(Error::INVALID_ARGUMENT);
    }
    let result = checked(syscall::invoke(
        crate::abi::SYS_READDIR,
        [
            path.as_ptr() as u64,
            path.len() as u64,
            output.as_mut_ptr() as u64,
            output.len() as u64,
            0,
            0,
        ],
    ))?;
    usize::try_from(result).map_err(|_| Error::INVALID_ARGUMENT)
}

pub fn write_file(path: &str, contents: &[u8]) -> Result<usize> {
    let path = path_bytes(path, crate::abi::MAX_MUTABLE_PATH)?;
    if contents.len() > crate::abi::MAX_WRITE_FILE_SIZE {
        return Err(Error::NO_SPACE);
    }
    let result = checked(syscall::invoke(
        crate::abi::SYS_WRITE_FILE,
        [
            path.as_ptr() as u64,
            path.len() as u64,
            contents.as_ptr() as u64,
            contents.len() as u64,
            0,
            0,
        ],
    ))?;
    usize::try_from(result).map_err(|_| Error::INVALID_ARGUMENT)
}

pub fn mkdir(path: &str, parents: bool) -> Result<()> {
    let path = path_bytes(path, crate::abi::MAX_MUTABLE_PATH)?;
    checked(syscall::invoke(
        crate::abi::SYS_MKDIR,
        [
            path.as_ptr() as u64,
            path.len() as u64,
            parents as u64,
            0,
            0,
            0,
        ],
    ))
    .map(|_| ())
}

pub fn touch(path: &str) -> Result<()> {
    let path = path_bytes(path, crate::abi::MAX_MUTABLE_PATH)?;
    checked(syscall::invoke(
        crate::abi::SYS_TOUCH,
        [path.as_ptr() as u64, path.len() as u64, 0, 0, 0, 0],
    ))
    .map(|_| ())
}

pub fn remove(path: &str, recursive: bool) -> Result<()> {
    let path = path_bytes(path, crate::abi::MAX_MUTABLE_PATH)?;
    checked(syscall::invoke(
        crate::abi::SYS_REMOVE,
        [
            path.as_ptr() as u64,
            path.len() as u64,
            recursive as u64,
            0,
            0,
            0,
        ],
    ))
    .map(|_| ())
}

pub struct DirectoryEntries<'a> {
    remaining: &'a [u8],
}

impl<'a> DirectoryEntries<'a> {
    pub const fn new(buffer: &'a [u8]) -> Self {
        Self { remaining: buffer }
    }
}

impl<'a> Iterator for DirectoryEntries<'a> {
    type Item = &'a [u8];

    fn next(&mut self) -> Option<Self::Item> {
        while !self.remaining.is_empty() {
            let end = self
                .remaining
                .iter()
                .position(|byte| *byte == b'\n')
                .unwrap_or(self.remaining.len());
            let (entry, rest) = self.remaining.split_at(end);
            self.remaining = rest.strip_prefix(b"\n").unwrap_or(rest);
            if !entry.is_empty() {
                return Some(entry);
            }
        }
        None
    }
}

#[cfg(test)]
mod tests {
    use super::DirectoryEntries;

    #[test]
    fn splits_newline_delimited_directory_names() {
        let mut names = DirectoryEntries::new(b"first\nsecond\n");
        assert_eq!(names.next(), Some(&b"first"[..]));
        assert_eq!(names.next(), Some(&b"second"[..]));
        assert_eq!(names.next(), None);
    }
}

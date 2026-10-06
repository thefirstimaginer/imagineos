use crate::{checked, syscall, Error, Result};

pub fn write(bytes: &[u8]) -> Result<usize> {
    if bytes.len() > crate::abi::MAX_CONSOLE_WRITE {
        return Err(Error::INVALID_ARGUMENT);
    }
    let result = checked(syscall::invoke(
        crate::abi::SYS_WRITE,
        [bytes.as_ptr() as u64, bytes.len() as u64, 0, 0, 0, 0],
    ))?;
    usize::try_from(result).map_err(|_| Error::INVALID_ARGUMENT)
}

pub fn write_all(mut bytes: &[u8]) -> Result<()> {
    while !bytes.is_empty() {
        let written = write(&bytes[..bytes.len().min(crate::abi::MAX_CONSOLE_WRITE)])?;
        if written == 0 {
            return Err(Error::from_errno(5));
        }
        bytes = &bytes[written..];
    }
    Ok(())
}

pub fn read_char() -> Result<char> {
    let value = checked(syscall::invoke(crate::abi::SYS_READ, [0; 6]))?;
    char::from_u32(value as u32).ok_or(Error::INVALID_ARGUMENT)
}

pub fn clear() -> Result<()> {
    checked(syscall::invoke(crate::abi::SYS_CLEAR, [0; 6])).map(|_| ())
}

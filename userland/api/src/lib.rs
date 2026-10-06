#![no_std]

pub use imagineos_abi as abi;

pub mod args;
pub mod console;
pub mod fs;
pub mod kernel_log;
pub mod legacy;
pub mod process;
pub mod shutdown;
pub mod signals;
pub mod syscall;

#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub struct Error {
    code: i32,
}

impl Error {
    pub const INVALID_ARGUMENT: Self = Self { code: 22 };
    pub const TOO_BIG: Self = Self { code: 7 };
    pub const NO_SPACE: Self = Self { code: 28 };

    pub const fn code(self) -> i32 {
        self.code
    }

    pub const fn from_errno(code: i32) -> Self {
        Self {
            code: code.saturating_abs(),
        }
    }
}

pub type Result<T> = core::result::Result<T, Error>;

pub(crate) fn checked(value: u64) -> Result<u64> {
    let signed = value as i64;
    if signed < 0 {
        Err(Error::from_errno(signed.saturating_abs() as i32))
    } else {
        Ok(value)
    }
}

pub(crate) fn path_bytes(path: &str, max_len: usize) -> Result<&[u8]> {
    let bytes = path.as_bytes();
    if bytes.is_empty() || bytes.len() > max_len || bytes.contains(&0) {
        return Err(Error::INVALID_ARGUMENT);
    }
    Ok(bytes)
}

#[cfg(test)]
mod tests {
    use super::{checked, path_bytes, Error};

    #[test]
    fn decodes_negative_errno_returns() {
        assert_eq!(checked((-14i64) as u64), Err(Error::from_errno(14)));
        assert_eq!(checked(4096), Ok(4096));
    }

    #[test]
    fn rejects_empty_long_and_nul_paths() {
        assert!(path_bytes("", 8).is_err());
        assert!(path_bytes("123456789", 8).is_err());
        assert!(path_bytes("a\0b", 8).is_err());
        assert_eq!(path_bytes("/ok", 8), Ok(&b"/ok"[..]));
    }
}

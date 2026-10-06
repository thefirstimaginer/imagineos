use imagineos_abi::UserArg;

use crate::{checked, path_bytes, syscall, Error, Result};

pub fn pid() -> Result<usize> {
    let value = checked(syscall::invoke(crate::abi::SYS_GETPID, [0; 6]))?;
    usize::try_from(value).map_err(|_| Error::INVALID_ARGUMENT)
}

pub fn yield_now() -> Result<()> {
    checked(syscall::invoke(crate::abi::SYS_YIELD, [0; 6])).map(|_| ())
}

pub fn exec(path: &str, arguments: &[&str], environment: &[&str]) -> Result<usize> {
    let path = path_bytes(path, crate::abi::MAX_EXEC_ITEM_SIZE)?;
    if arguments.len() > crate::abi::MAX_EXEC_ARGS || environment.len() > crate::abi::MAX_EXEC_ENV {
        return Err(Error::TOO_BIG);
    }

    let mut argument_records = [UserArg::default(); crate::abi::MAX_EXEC_ARGS];
    for (record, value) in argument_records.iter_mut().zip(arguments) {
        *record = user_arg(value)?;
    }
    let mut environment_records = [UserArg::default(); crate::abi::MAX_EXEC_ENV];
    for (record, value) in environment_records.iter_mut().zip(environment) {
        *record = user_arg(value)?;
    }

    let result = checked(syscall::invoke(
        crate::abi::SYS_EXEC,
        [
            path.as_ptr() as u64,
            path.len() as u64,
            argument_records.as_ptr() as u64,
            arguments.len() as u64,
            environment_records.as_ptr() as u64,
            environment.len() as u64,
        ],
    ))?;
    usize::try_from(result).map_err(|_| Error::INVALID_ARGUMENT)
}

pub fn exit(status: i32) -> ! {
    let _ = status;
    let _ = syscall::invoke(crate::abi::SYS_EXIT, [0; 6]);
    loop {
        core::hint::spin_loop();
    }
}

fn user_arg(value: &str) -> Result<UserArg> {
    let bytes = value.as_bytes();
    if bytes.len() > crate::abi::MAX_EXEC_ITEM_SIZE {
        return Err(Error::TOO_BIG);
    }
    if bytes.contains(&0) {
        return Err(Error::INVALID_ARGUMENT);
    }
    Ok(UserArg {
        address: bytes.as_ptr() as u64,
        length: bytes.len() as u64,
    })
}

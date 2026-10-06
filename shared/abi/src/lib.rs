#![no_std]

pub const ABI_VERSION: u64 = 2;

pub const SYS_WRITE: u64 = 1;
pub const SYS_READ: u64 = 2;
pub const SYS_YIELD: u64 = 3;
pub const SYS_EXIT: u64 = 4;
pub const SYS_GETPID: u64 = 5;
pub const SYS_CLEAR: u64 = 6;
pub const SYS_EXEC: u64 = 7;
pub const SYS_ISDIR: u64 = 8;
pub const SYS_ISFILE: u64 = 9;
pub const SYS_READ_FILE: u64 = 10;
pub const SYS_READDIR: u64 = 11;
pub const SYS_MKDIR: u64 = 12;
pub const SYS_TOUCH: u64 = 13;
pub const SYS_REMOVE: u64 = 14;
pub const SYS_WRITE_FILE: u64 = 15;
pub const SYS_ABI_VERSION: u64 = 16;
pub const SYS_OPEN: u64 = 17;
pub const SYS_READ_FD: u64 = 18;
pub const SYS_WRITE_FD: u64 = 19;
pub const SYS_CLOSE: u64 = 20;
pub const SYS_DISK_COUNT: u64 = 21;
pub const SYS_DISK_SECTORS: u64 = 22;
pub const SYS_INSTALL_DISK: u64 = 23;
pub const SYS_STAT: u64 = 24;
pub const SYS_DMESG: u64 = 25;
pub const SYS_SHUTDOWN_REQUEST: u64 = 26;
pub const SYS_SHUTDOWN_POLL: u64 = 27;
pub const SYS_POWER_OFF: u64 = 28;
pub const SYS_KILL: u64 = 29;
pub const SYS_SIGACTION: u64 = 30;
pub const SYS_SIGRETURN: u64 = 31;
pub const SYS_PROCESS_LIST: u64 = 32;
pub const SYS_AUTHENTICATE: u64 = 33;
pub const SYS_GETIDENTITY: u64 = 34;
pub const SYS_INSTALL_DISK_CONFIG: u64 = 35;

pub const SIGNAL_HUP: u64 = 1;
pub const SIGNAL_INT: u64 = 2;
pub const SIGNAL_KILL: u64 = 9;
pub const SIGNAL_SEGV: u64 = 11;
pub const SIGNAL_TERM: u64 = 15;
pub const SIGNAL_CONT: u64 = 18;
pub const SIGNAL_STOP: u64 = 19;
pub const SIGNAL_DEFAULT: u64 = 0;
pub const SIGNAL_IGNORE: u64 = 1;

pub const PROCESS_RUNNING: u32 = 1;
pub const PROCESS_STOPPED: u32 = 2;
pub const MAX_PROCESSES: usize = 8;
pub const PROCESS_NAME_SIZE: usize = 64;
pub const ACCOUNT_NAME_SIZE: usize = 32;
pub const ACCOUNT_PASSWORD_SIZE: usize = 64;
pub const HOSTNAME_SIZE: usize = 64;

pub const FD_STDIN: u64 = 0;
pub const FD_STDOUT: u64 = 1;
pub const FD_STDERR: u64 = 2;
pub const MAX_OPEN_FDS: usize = 16;

pub const OPEN_READ: u64 = 1 << 0;
pub const OPEN_WRITE: u64 = 1 << 1;
pub const OPEN_CREATE: u64 = 1 << 2;
pub const OPEN_TRUNCATE: u64 = 1 << 3;
pub const OPEN_APPEND: u64 = 1 << 4;

pub const MAX_EXEC_ARGS: usize = 12;
pub const MAX_EXEC_ENV: usize = 12;
pub const MAX_EXEC_ITEM_SIZE: usize = 128;
pub const MAX_CONSOLE_WRITE: usize = 512;
pub const MAX_PATH_QUERY: usize = 128;
pub const MAX_MUTABLE_PATH: usize = 256;
pub const MAX_READ_BUFFER: usize = 4096;
pub const MAX_WRITE_FILE_SIZE: usize = 4096;

#[repr(C)]
#[derive(Clone, Copy, Debug, Default, PartialEq, Eq)]
pub struct UserArg {
    pub address: u64,
    pub length: u64,
}

#[repr(C)]
#[derive(Clone, Copy, Debug, Default, PartialEq, Eq)]
pub struct UserStat {
    pub size: u64,
    pub mode: u32,
    pub uid: u32,
    pub gid: u32,
    pub kind: u32,
}

#[repr(C)]
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub struct ProcessInfo {
    pub pid: u64,
    pub state: u32,
    pub pending_signals: u32,
    pub name: [u8; PROCESS_NAME_SIZE],
}

#[repr(C)]
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub struct InstallConfig {
    pub username_length: u8,
    pub password_length: u8,
    pub hostname_length: u8,
    pub add_user: u8,
    pub administrator: u8,
    pub reserved: [u8; 3],
    pub username: [u8; ACCOUNT_NAME_SIZE],
    pub password: [u8; ACCOUNT_PASSWORD_SIZE],
    pub hostname: [u8; HOSTNAME_SIZE],
}

impl Default for InstallConfig {
    fn default() -> Self {
        Self {
            username_length: 0,
            password_length: 0,
            hostname_length: 0,
            add_user: 0,
            administrator: 0,
            reserved: [0; 3],
            username: [0; ACCOUNT_NAME_SIZE],
            password: [0; ACCOUNT_PASSWORD_SIZE],
            hostname: [0; HOSTNAME_SIZE],
        }
    }
}

#[repr(C)]
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub struct UserIdentity {
    pub uid: u32,
    pub gid: u32,
    pub is_admin: u32,
    pub username_length: u32,
    pub username: [u8; ACCOUNT_NAME_SIZE],
    pub hostname_length: u32,
    pub hostname: [u8; HOSTNAME_SIZE],
}

impl Default for UserIdentity {
    fn default() -> Self {
        Self {
            uid: 0,
            gid: 0,
            is_admin: 0,
            username_length: 0,
            username: [0; ACCOUNT_NAME_SIZE],
            hostname_length: 0,
            hostname: [0; HOSTNAME_SIZE],
        }
    }
}

impl Default for ProcessInfo {
    fn default() -> Self {
        Self {
            pid: 0,
            state: 0,
            pending_signals: 0,
            name: [0; PROCESS_NAME_SIZE],
        }
    }
}

impl UserArg {
    pub const EMPTY: Self = Self {
        address: 0,
        length: 0,
    };
}

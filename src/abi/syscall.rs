use core::arch::asm;
use core::cell::UnsafeCell;

use crate::keyboard::Keyboard;
use crate::process::UserArg;
use crate::{framebuffer, process};

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
pub const SYS_OPEN: u64 = 16;
pub const SYS_READ_FD: u64 = 17;
pub const SYS_WRITE_FD: u64 = 18;
pub const SYS_CLOSE: u64 = 19;
pub const SYS_LSEEK: u64 = 20;
pub const SYS_STAT: u64 = 21;
pub const SYS_FSTAT: u64 = 22;

const PROCESS_SLOT_COUNT: usize = 4;
const OPEN_FILE_COUNT: usize = 12;
const FIRST_USER_FD: usize = 3;
const OPEN_WRITE_ONLY: u32 = 1;
const OPEN_CREATE: u32 = 0x40;
const OPEN_EXCLUSIVE: u32 = 0x80;
const OPEN_TRUNCATE: u32 = 0x200;
const OPEN_APPEND: u32 = 0x400;

#[derive(Clone, Copy)]
struct OpenFile {
    active: bool,
    path: [u8; 256],
    path_length: usize,
    offset: usize,
    flags: u32,
}

impl OpenFile {
    const EMPTY: Self = Self {
        active: false,
        path: [0; 256],
        path_length: 0,
        offset: 0,
        flags: 0,
    };

    fn path(&self) -> &str {
        core::str::from_utf8(&self.path[..self.path_length]).unwrap_or("")
    }
}

struct OpenFiles(UnsafeCell<[[OpenFile; OPEN_FILE_COUNT]; PROCESS_SLOT_COUNT]>);
unsafe impl Sync for OpenFiles {}
static OPEN_FILES: OpenFiles = OpenFiles(UnsafeCell::new(
    [[OpenFile::EMPTY; OPEN_FILE_COUNT]; PROCESS_SLOT_COUNT],
));

#[repr(C)]
#[derive(Clone, Copy)]
struct PosixTimespec {
    seconds: i64,
    nanoseconds: i64,
}

#[repr(C)]
#[derive(Clone, Copy)]
struct PosixStat {
    device: u64,
    inode: u64,
    links: u64,
    mode: u32,
    uid: u32,
    gid: u32,
    pad: u32,
    special_device: u64,
    size: i64,
    block_size: i64,
    blocks: i64,
    access_time: PosixTimespec,
    modification_time: PosixTimespec,
    change_time: PosixTimespec,
    unused: [i64; 3],
}

pub fn reset_open_files(slot: usize) {
    if slot < PROCESS_SLOT_COUNT {
        unsafe {
            (*OPEN_FILES.0.get())[slot] = [OpenFile::EMPTY; OPEN_FILE_COUNT];
        }
    }
}

struct SharedKeyboard(UnsafeCell<Keyboard>);
unsafe impl Sync for SharedKeyboard {}
static KEYBOARD: SharedKeyboard = SharedKeyboard(UnsafeCell::new(Keyboard::new()));

#[repr(C)]
pub struct TrapFrame {
    pub r15: u64,
    pub r14: u64,
    pub r13: u64,
    pub r12: u64,
    pub r11: u64,
    pub r10: u64,
    pub r9: u64,
    pub r8: u64,
    pub rbp: u64,
    pub rdi: u64,
    pub rsi: u64,
    pub rdx: u64,
    pub rcx: u64,
    pub rbx: u64,
    pub rax: u64,
    pub rip: u64,
    pub cs: u64,
    pub rflags: u64,
    pub rsp: u64,
    pub ss: u64,
}

#[no_mangle]
extern "C" fn dreamcore_syscall_dispatch(frame: *mut TrapFrame) -> *mut TrapFrame {
    let Some(frame_ref) = (unsafe { frame.as_mut() }) else {
        return frame;
    };
    match frame_ref.rax {
        SYS_YIELD => {
            frame_ref.rax = 0;
            process::yield_current(frame)
        }
        SYS_EXIT => process::exit_current(frame),
        SYS_GETPID => {
            frame_ref.rax = process::current_pid() as u64;
            frame
        }
        SYS_WRITE => {
            let length = (frame_ref.rsi as usize).min(512);
            let mut buffer = [0u8; 512];
            if process::copy_from_current_user(frame_ref.rdi, &mut buffer[..length]) {
                crate::console_write_bytes(&buffer[..length]);
                frame_ref.rax = length as u64;
            } else {
                frame_ref.rax = (-14i64) as u64;
            }
            frame
        }
        SYS_READ => {
            frame_ref.rax = read_character() as u64;
            frame
        }
        SYS_OPEN => {
            let mut path_buffer = [0u8; 256];
            let path = match copy_user_path(frame_ref.rdi, frame_ref.rsi, &mut path_buffer) {
                Ok(path) => path,
                Err(error) => {
                    frame_ref.rax = error as u64;
                    return frame;
                }
            };
            frame_ref.rax = open_user_file(path, frame_ref.rdx as u32) as u64;
            frame
        }
        SYS_READ_FD => {
            frame_ref.rax = read_user_fd(
                frame_ref.rdi as usize,
                frame_ref.rsi,
                frame_ref.rdx as usize,
            ) as u64;
            frame
        }
        SYS_WRITE_FD => {
            frame_ref.rax = write_user_fd(
                frame_ref.rdi as usize,
                frame_ref.rsi,
                frame_ref.rdx as usize,
            ) as u64;
            frame
        }
        SYS_CLOSE => {
            frame_ref.rax = close_user_fd(frame_ref.rdi as usize) as u64;
            frame
        }
        SYS_LSEEK => {
            frame_ref.rax = seek_user_fd(
                frame_ref.rdi as usize,
                frame_ref.rsi as i64,
                frame_ref.rdx as usize,
            ) as u64;
            frame
        }
        SYS_STAT => {
            frame_ref.rax = stat_user_path(frame_ref.rdi, frame_ref.rsi, frame_ref.rdx) as u64;
            frame
        }
        SYS_FSTAT => {
            frame_ref.rax = fstat_user_fd(frame_ref.rdi as usize, frame_ref.rsi) as u64;
            frame
        }
        SYS_CLEAR => {
            framebuffer::clear();
            frame_ref.rax = 0;
            frame
        }
        SYS_EXEC => {
            let length = (frame_ref.rsi as usize).min(128);
            let mut path = [0u8; 128];
            if !process::copy_from_current_user(frame_ref.rdi, &mut path[..length]) {
                frame_ref.rax = (-14i64) as u64;
                return frame;
            }
            let Ok(path) = core::str::from_utf8(&path[..length]) else {
                frame_ref.rax = (-22i64) as u64;
                return frame;
            };
            let argument_count = frame_ref.r10 as usize;
            if argument_count > process::MAX_EXEC_ARGS {
                frame_ref.rax = (-7i64) as u64;
                return frame;
            }
            let mut user_arguments = [UserArg::EMPTY; process::MAX_EXEC_ARGS];
            let metadata_length = argument_count * core::mem::size_of::<UserArg>();
            let metadata = unsafe {
                core::slice::from_raw_parts_mut(
                    user_arguments.as_mut_ptr().cast::<u8>(),
                    metadata_length,
                )
            };
            if !process::copy_from_current_user(frame_ref.rdx, metadata) {
                frame_ref.rax = (-14i64) as u64;
                return frame;
            }

            let mut argument_storage = [[0u8; 128]; process::MAX_EXEC_ARGS];
            let mut argument_lengths = [0usize; process::MAX_EXEC_ARGS];
            for index in 0..argument_count {
                let Ok(length) = usize::try_from(user_arguments[index].length) else {
                    frame_ref.rax = (-7i64) as u64;
                    return frame;
                };
                if length > argument_storage[index].len()
                    || !process::copy_from_current_user(
                        user_arguments[index].address,
                        &mut argument_storage[index][..length],
                    )
                {
                    frame_ref.rax = (-14i64) as u64;
                    return frame;
                }
                argument_lengths[index] = length;
            }
            let mut arguments: [&[u8]; process::MAX_EXEC_ARGS] = [&[]; process::MAX_EXEC_ARGS];
            for index in 0..argument_count {
                arguments[index] = &argument_storage[index][..argument_lengths[index]];
            }

            let environment_count = frame_ref.r9 as usize;
            if environment_count > process::MAX_EXEC_ENV {
                frame_ref.rax = (-7i64) as u64;
                return frame;
            }
            let mut user_environment = [UserArg::EMPTY; process::MAX_EXEC_ENV];
            let metadata_length = environment_count * core::mem::size_of::<UserArg>();
            let metadata = unsafe {
                core::slice::from_raw_parts_mut(
                    user_environment.as_mut_ptr().cast::<u8>(),
                    metadata_length,
                )
            };
            if !process::copy_from_current_user(frame_ref.r8, metadata) {
                frame_ref.rax = (-14i64) as u64;
                return frame;
            }
            let mut environment_storage = [[0u8; 128]; process::MAX_EXEC_ENV];
            let mut environment_lengths = [0usize; process::MAX_EXEC_ENV];
            for index in 0..environment_count {
                let Ok(length) = usize::try_from(user_environment[index].length) else {
                    frame_ref.rax = (-7i64) as u64;
                    return frame;
                };
                if length > environment_storage[index].len()
                    || !process::copy_from_current_user(
                        user_environment[index].address,
                        &mut environment_storage[index][..length],
                    )
                {
                    frame_ref.rax = (-14i64) as u64;
                    return frame;
                }
                environment_lengths[index] = length;
            }
            let mut environment: [&[u8]; process::MAX_EXEC_ENV] = [&[]; process::MAX_EXEC_ENV];
            for index in 0..environment_count {
                environment[index] = &environment_storage[index][..environment_lengths[index]];
            }
            process::spawn_current(
                path,
                &arguments[..argument_count],
                &environment[..environment_count],
                frame,
            )
        }
        SYS_ISDIR => {
            let length = (frame_ref.rsi as usize).min(128);
            let mut path = [0u8; 128];
            if !process::copy_from_current_user(frame_ref.rdi, &mut path[..length]) {
                frame_ref.rax = (-14i64) as u64;
                return frame;
            }
            let Ok(path) = core::str::from_utf8(&path[..length]) else {
                frame_ref.rax = (-22i64) as u64;
                return frame;
            };
            frame_ref.rax = crate::ramfs::is_directory(path) as u64;
            frame
        }
        SYS_ISFILE => {
            let length = (frame_ref.rsi as usize).min(128);
            let mut path = [0u8; 128];
            if !process::copy_from_current_user(frame_ref.rdi, &mut path[..length]) {
                frame_ref.rax = (-14i64) as u64;
                return frame;
            }
            let Ok(path) = core::str::from_utf8(&path[..length]) else {
                frame_ref.rax = (-22i64) as u64;
                return frame;
            };
            frame_ref.rax = crate::ramfs::is_file(path) as u64;
            frame
        }
        SYS_READ_FILE | SYS_READDIR => {
            let path_length = (frame_ref.rsi as usize).min(128);
            let capacity = (frame_ref.r10 as usize).min(4096);
            let mut path = [0u8; 128];
            if !process::copy_from_current_user(frame_ref.rdi, &mut path[..path_length]) {
                frame_ref.rax = (-14i64) as u64;
                return frame;
            }
            let Ok(path) = core::str::from_utf8(&path[..path_length]) else {
                frame_ref.rax = (-22i64) as u64;
                return frame;
            };
            let mut buffer = [0u8; 4096];
            let length = if frame_ref.rax == SYS_READ_FILE {
                match crate::ramfs::read_file_into(path, &mut buffer[..capacity]) {
                    Ok(length) => length,
                    Err(crate::ramfs::FsError::NotFound) => {
                        frame_ref.rax = (-2i64) as u64;
                        return frame;
                    }
                    Err(crate::ramfs::FsError::NoSpace) => {
                        frame_ref.rax = (-75i64) as u64;
                        return frame;
                    }
                    Err(error) => {
                        frame_ref.rax = fs_error_code(error) as u64;
                        return frame;
                    }
                }
            } else {
                let Some(length) = crate::ramfs::list_directory(path, &mut buffer[..capacity])
                else {
                    frame_ref.rax = (-75i64) as u64;
                    return frame;
                };
                length
            };
            if !process::copy_to_current_user(frame_ref.rdx, &buffer[..length]) {
                frame_ref.rax = (-14i64) as u64;
            } else {
                frame_ref.rax = length as u64;
            }
            frame
        }
        SYS_WRITE_FILE => {
            let mut path_buffer = [0u8; 256];
            let path = match copy_user_path(frame_ref.rdi, frame_ref.rsi, &mut path_buffer) {
                Ok(path) => path,
                Err(error) => {
                    frame_ref.rax = error as u64;
                    return frame;
                }
            };
            let length = (frame_ref.r10 as usize).min(4097);
            if length > 4096 {
                frame_ref.rax = (-75i64) as u64;
                return frame;
            }
            let mut contents = [0u8; 4096];
            if !process::copy_from_current_user(frame_ref.rdx, &mut contents[..length]) {
                frame_ref.rax = (-14i64) as u64;
                return frame;
            }
            frame_ref.rax = crate::ramfs::write_file(path, &contents[..length])
                .map_or_else(fs_error_code, |_| length as i64) as u64;
            frame
        }
        SYS_MKDIR | SYS_TOUCH | SYS_REMOVE => {
            let mut path_buffer = [0u8; 256];
            let path = match copy_user_path(frame_ref.rdi, frame_ref.rsi, &mut path_buffer) {
                Ok(path) => path,
                Err(error) => {
                    frame_ref.rax = error as u64;
                    return frame;
                }
            };
            let result = match frame_ref.rax {
                SYS_MKDIR if frame_ref.rdx != 0 => {
                    crate::ramfs::create_directory_with_parents(path, true)
                }
                SYS_MKDIR => crate::ramfs::create_directory(path),
                SYS_TOUCH => crate::ramfs::create_file(path),
                SYS_REMOVE => crate::ramfs::remove(path, frame_ref.rdx != 0),
                _ => unreachable!(),
            };
            frame_ref.rax = result.map_or_else(fs_error_code, |_| 0) as u64;
            frame
        }
        _ => {
            frame_ref.rax = u64::MAX;
            frame
        }
    }
}

fn open_user_file(path: &str, flags: u32) -> i64 {
    let access = flags & 3;
    if access == 3 {
        return -22;
    }
    if crate::ramfs::is_directory(path) {
        return -21;
    }
    let exists = crate::ramfs::is_file(path);
    if exists && flags & OPEN_CREATE != 0 && flags & OPEN_EXCLUSIVE != 0 {
        return -17;
    }
    if !exists {
        if flags & OPEN_CREATE == 0 {
            return -2;
        }
        if let Err(error) = crate::ramfs::create_file(path) {
            return fs_error_code(error);
        }
    }
    if flags & OPEN_TRUNCATE != 0 {
        if access == 0 {
            return -22;
        }
        if let Err(error) = crate::ramfs::write_file(path, &[]) {
            return fs_error_code(error);
        }
    }

    let slot = process::current_slot();
    if slot >= PROCESS_SLOT_COUNT {
        return -24;
    }
    let files = unsafe { &mut (*OPEN_FILES.0.get())[slot] };
    let Some(index) = files.iter().position(|file| !file.active) else {
        return -24;
    };
    let mut file = OpenFile::EMPTY;
    file.path[..path.len()].copy_from_slice(path.as_bytes());
    file.path_length = path.len();
    file.flags = flags;
    file.active = true;
    files[index] = file;
    (index + FIRST_USER_FD) as i64
}

fn read_user_fd(descriptor: usize, destination: u64, count: usize) -> i64 {
    if count == 0 {
        return 0;
    }
    if descriptor == 0 {
        let character = read_character() as u32;
        let mut encoded = [0u8; 4];
        let length = encode_utf8(character, &mut encoded);
        if count < length {
            return -22;
        }
        return if process::copy_to_current_user(destination, &encoded[..length]) {
            length as i64
        } else {
            -14
        };
    }
    if descriptor < FIRST_USER_FD || descriptor >= FIRST_USER_FD + OPEN_FILE_COUNT {
        return -9;
    }
    let slot = process::current_slot();
    let files = unsafe { &mut (*OPEN_FILES.0.get())[slot] };
    let file = &mut files[descriptor - FIRST_USER_FD];
    if !file.active || file.flags & 3 == OPEN_WRITE_ONLY {
        return -9;
    }
    let path = file.path();
    if crate::ramfs::is_directory(path) {
        return -21;
    }
    let mut contents = [0u8; 4096];
    let length = match crate::ramfs::read_file_into(path, &mut contents) {
        Ok(length) => length,
        Err(crate::ramfs::FsError::NoSpace) => return -75,
        Err(error) => return fs_error_code(error),
    };
    if file.offset >= length {
        return 0;
    }
    let amount = count.min(4096).min(length - file.offset);
    if !process::copy_to_current_user(destination, &contents[file.offset..file.offset + amount]) {
        return -14;
    }
    file.offset += amount;
    amount as i64
}

fn write_user_fd(descriptor: usize, source: u64, count: usize) -> i64 {
    if count == 0 {
        return 0;
    }
    let amount = count.min(4096);
    let mut input = [0u8; 4096];
    if !process::copy_from_current_user(source, &mut input[..amount]) {
        return -14;
    }
    if descriptor == 1 || descriptor == 2 {
        crate::console_write_bytes(&input[..amount]);
        return amount as i64;
    }
    if descriptor < FIRST_USER_FD || descriptor >= FIRST_USER_FD + OPEN_FILE_COUNT {
        return -9;
    }
    let slot = process::current_slot();
    let files = unsafe { &mut (*OPEN_FILES.0.get())[slot] };
    let file = &mut files[descriptor - FIRST_USER_FD];
    if !file.active || file.flags & 3 == 0 {
        return -9;
    }
    let path = file.path();
    let mut contents = [0u8; 4096];
    let old_length = match crate::ramfs::read_file_into(path, &mut contents) {
        Ok(length) => length,
        Err(crate::ramfs::FsError::NotFound) => 0,
        Err(error) => return fs_error_code(error),
    };
    let offset = if file.flags & OPEN_APPEND != 0 {
        old_length
    } else {
        file.offset
    };
    if offset > contents.len() {
        return -28;
    }
    let written = amount.min(contents.len() - offset);
    contents[offset..offset + written].copy_from_slice(&input[..written]);
    let new_length = old_length.max(offset + written);
    if let Err(error) = crate::ramfs::write_file(path, &contents[..new_length]) {
        return fs_error_code(error);
    }
    file.offset = offset + written;
    written as i64
}

fn close_user_fd(descriptor: usize) -> i64 {
    if descriptor < FIRST_USER_FD || descriptor >= FIRST_USER_FD + OPEN_FILE_COUNT {
        return -9;
    }
    let slot = process::current_slot();
    let files = unsafe { &mut (*OPEN_FILES.0.get())[slot] };
    let file = &mut files[descriptor - FIRST_USER_FD];
    if !file.active {
        return -9;
    }
    *file = OpenFile::EMPTY;
    0
}

fn seek_user_fd(descriptor: usize, offset: i64, whence: usize) -> i64 {
    if descriptor < FIRST_USER_FD || descriptor >= FIRST_USER_FD + OPEN_FILE_COUNT {
        return -9;
    }
    let slot = process::current_slot();
    let files = unsafe { &mut (*OPEN_FILES.0.get())[slot] };
    let file = &mut files[descriptor - FIRST_USER_FD];
    if !file.active {
        return -9;
    }
    let base = match whence {
        0 => 0i64,
        1 => file.offset as i64,
        2 => {
            let mut contents = [0u8; 4096];
            match crate::ramfs::read_file_into(file.path(), &mut contents) {
                Ok(length) => length as i64,
                Err(error) => return fs_error_code(error),
            }
        }
        _ => return -22,
    };
    let Some(position) = base.checked_add(offset) else {
        return -22;
    };
    if position < 0 || position > 4096 {
        return -22;
    }
    file.offset = position as usize;
    position
}

fn stat_user_path(address: u64, length: u64, destination: u64) -> i64 {
    let mut path_buffer = [0u8; 256];
    let path = match copy_user_path(address, length, &mut path_buffer) {
        Ok(path) => path,
        Err(error) => return error,
    };
    let metadata = match stat_path(path) {
        Ok(metadata) => metadata,
        Err(error) => return error,
    };
    copy_stat_to_user(destination, &metadata)
}

fn fstat_user_fd(descriptor: usize, destination: u64) -> i64 {
    let metadata = if descriptor < FIRST_USER_FD {
        PosixStat {
            device: 1,
            inode: descriptor as u64 + 1,
            links: 1,
            mode: 0o020666,
            uid: 0,
            gid: 0,
            pad: 0,
            special_device: descriptor as u64,
            size: 0,
            block_size: 4096,
            blocks: 0,
            access_time: PosixTimespec { seconds: 0, nanoseconds: 0 },
            modification_time: PosixTimespec { seconds: 0, nanoseconds: 0 },
            change_time: PosixTimespec { seconds: 0, nanoseconds: 0 },
            unused: [0; 3],
        }
    } else {
        if descriptor >= FIRST_USER_FD + OPEN_FILE_COUNT {
            return -9;
        }
        let slot = process::current_slot();
        let files = unsafe { &(*OPEN_FILES.0.get())[slot] };
        let file = files[descriptor - FIRST_USER_FD];
        if !file.active {
            return -9;
        }
        match stat_path(file.path()) {
            Ok(metadata) => metadata,
            Err(error) => return error,
        }
    };
    copy_stat_to_user(destination, &metadata)
}

fn stat_path(path: &str) -> Result<PosixStat, i64> {
    let directory = crate::ramfs::is_directory(path);
    let file = crate::ramfs::is_file(path);
    if !directory && !file {
        return Err(-2);
    }
    let size = if file {
        crate::ramfs::file_size(path).ok_or(-2)?
    } else {
        0
    };
    let mut inode = 0xcbf2_9ce4_8422_2325u64;
    for byte in path.as_bytes() {
        inode = (inode ^ *byte as u64).wrapping_mul(0x100_0000_01b3);
    }
    Ok(PosixStat {
        device: 1,
        inode,
        links: 1,
        mode: if directory { 0o040755 } else { 0o100644 },
        uid: 0,
        gid: 0,
        pad: 0,
        special_device: 0,
        size: size as i64,
        block_size: 4096,
        blocks: size.div_ceil(512) as i64,
        access_time: PosixTimespec { seconds: 0, nanoseconds: 0 },
        modification_time: PosixTimespec { seconds: 0, nanoseconds: 0 },
        change_time: PosixTimespec { seconds: 0, nanoseconds: 0 },
        unused: [0; 3],
    })
}

fn copy_stat_to_user(destination: u64, metadata: &PosixStat) -> i64 {
    let bytes = unsafe {
        core::slice::from_raw_parts(
            (metadata as *const PosixStat).cast::<u8>(),
            core::mem::size_of::<PosixStat>(),
        )
    };
    if process::copy_to_current_user(destination, bytes) {
        0
    } else {
        -14
    }
}

fn encode_utf8(character: u32, output: &mut [u8; 4]) -> usize {
    if character < 0x80 {
        output[0] = character as u8;
        1
    } else if character < 0x800 {
        output[0] = 0xc0 | (character >> 6) as u8;
        output[1] = 0x80 | (character & 0x3f) as u8;
        2
    } else if character < 0x10000 {
        output[0] = 0xe0 | (character >> 12) as u8;
        output[1] = 0x80 | ((character >> 6) & 0x3f) as u8;
        output[2] = 0x80 | (character & 0x3f) as u8;
        3
    } else {
        output[0] = 0xf0 | (character >> 18) as u8;
        output[1] = 0x80 | ((character >> 12) & 0x3f) as u8;
        output[2] = 0x80 | ((character >> 6) & 0x3f) as u8;
        output[3] = 0x80 | (character & 0x3f) as u8;
        4
    }
}

fn copy_user_path<'a>(
    address: u64,
    length: u64,
    buffer: &'a mut [u8; 256],
) -> Result<&'a str, i64> {
    let length = usize::try_from(length).map_err(|_| -22i64)?;
    if length == 0 || length > buffer.len() {
        return Err(-22);
    }
    if !process::copy_from_current_user(address, &mut buffer[..length]) {
        return Err(-14);
    }
    core::str::from_utf8(&buffer[..length]).map_err(|_| -22)
}

fn fs_error_code(error: crate::ramfs::FsError) -> i64 {
    match error {
        crate::ramfs::FsError::InvalidPath => -22,
        crate::ramfs::FsError::NotFound => -2,
        crate::ramfs::FsError::NotDirectory => -20,
        crate::ramfs::FsError::IsDirectory => -21,
        crate::ramfs::FsError::AlreadyExists => -17,
        crate::ramfs::FsError::DirectoryNotEmpty => -39,
        crate::ramfs::FsError::NoSpace => -28,
    }
}

fn read_character() -> u32 {
    const BLINK_INTERVAL: u64 = 750_000_000;
    let mut last_blink = read_tsc();
    framebuffer::set_cursor_visible(true);
    let mut cursor_visible = true;
    loop {
        if let Some(character) = unsafe { (&mut *KEYBOARD.0.get()).poll_char() } {
            framebuffer::set_cursor_visible(false);
            return character as u32;
        }
        if let Some(character) = serial_read_char() {
            framebuffer::set_cursor_visible(false);
            return character as u32;
        }
        let now = read_tsc();
        if now.wrapping_sub(last_blink) >= BLINK_INTERVAL {
            cursor_visible = !cursor_visible;
            framebuffer::set_cursor_visible(cursor_visible);
            last_blink = now;
        }
        core::hint::spin_loop();
    }
}

fn read_tsc() -> u64 {
    let low: u32;
    let high: u32;
    unsafe {
        asm!(
            "rdtsc",
            out("eax") low,
            out("edx") high,
            options(nomem, nostack, preserves_flags)
        );
    }
    ((high as u64) << 32) | low as u64
}

fn serial_read_char() -> Option<char> {
    unsafe {
        if in_port(0x3fd) & 1 != 0 {
            Some(in_port(0x3f8) as char)
        } else {
            None
        }
    }
}

unsafe fn in_port(port: u16) -> u8 {
    let value: u8;
    asm!("in al, dx", in("dx") port, out("al") value, options(nomem, nostack, preserves_flags));
    value
}

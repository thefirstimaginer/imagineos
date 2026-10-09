use core::arch::asm;
use core::cell::UnsafeCell;
use core::mem::size_of;
use core::ptr;

use crate::elf::{Elf64, Error as ElfError};
use crate::gdt;
use crate::paging::{self, AddressSpace, MapError};
use crate::syscall::TrapFrame;
pub use imagineos_abi::UserArg;

const MAX_PROCESSES: usize = imagineos_abi::MAX_PROCESSES;
// The synchronous DFS installer nests large journal transaction buffers.
const KERNEL_STACK_SIZE: usize = 64 * 1024;
const USER_STACK_SIZE: u64 = 8 * 4096;
pub const USER_STACK_TOP: u64 = 0x0000_7fff_ffff_0000;
const PAGE_SIZE: u64 = 4096;
pub const MAX_EXEC_ARGS: usize = imagineos_abi::MAX_EXEC_ARGS;
pub const MAX_EXEC_ENV: usize = imagineos_abi::MAX_EXEC_ENV;
pub const MAX_OPEN_FDS: usize = imagineos_abi::MAX_OPEN_FDS;

#[derive(Clone, Copy, PartialEq, Eq)]
pub enum DescriptorKind {
    Closed,
    Stdin,
    Stdout,
    Stderr,
    File,
}

#[derive(Clone, Copy)]
pub struct Descriptor {
    pub kind: DescriptorKind,
    pub path: [u8; imagineos_abi::MAX_MUTABLE_PATH],
    pub path_length: usize,
    pub offset: usize,
    pub flags: u64,
    pub input: [u8; 4],
    pub input_length: usize,
    pub input_offset: usize,
}

impl Descriptor {
    const CLOSED: Self = Self::new(DescriptorKind::Closed);

    const fn new(kind: DescriptorKind) -> Self {
        Self {
            kind,
            path: [0; imagineos_abi::MAX_MUTABLE_PATH],
            path_length: 0,
            offset: 0,
            flags: 0,
            input: [0; 4],
            input_length: 0,
            input_offset: 0,
        }
    }

    pub fn file(path: &str, flags: u64, offset: usize) -> Self {
        let mut descriptor = Self::new(DescriptorKind::File);
        descriptor.path[..path.len()].copy_from_slice(path.as_bytes());
        descriptor.path_length = path.len();
        descriptor.flags = flags;
        descriptor.offset = offset;
        descriptor
    }
}

#[derive(Clone, Copy)]
pub struct DescriptorTable([Descriptor; MAX_OPEN_FDS]);

impl DescriptorTable {
    const fn new() -> Self {
        let mut descriptors = [Descriptor::CLOSED; MAX_OPEN_FDS];
        descriptors[0] = Descriptor::new(DescriptorKind::Stdin);
        descriptors[1] = Descriptor::new(DescriptorKind::Stdout);
        descriptors[2] = Descriptor::new(DescriptorKind::Stderr);
        Self(descriptors)
    }

    fn allocate(&mut self, descriptor: Descriptor) -> Option<usize> {
        let slot = (3..self.0.len()).find(|&index| self.0[index].kind == DescriptorKind::Closed)?;
        self.0[slot] = descriptor;
        Some(slot)
    }
}

struct SharedDescriptorTables(UnsafeCell<[DescriptorTable; MAX_PROCESSES]>);
unsafe impl Sync for SharedDescriptorTables {}
static DESCRIPTOR_TABLES: SharedDescriptorTables =
    SharedDescriptorTables(UnsafeCell::new([DescriptorTable::new(); MAX_PROCESSES]));

#[repr(align(16))]
struct KernelStacks([[u8; KERNEL_STACK_SIZE]; MAX_PROCESSES]);

struct SharedStacks(UnsafeCell<KernelStacks>);
unsafe impl Sync for SharedStacks {}
static KERNEL_STACKS: SharedStacks = SharedStacks(UnsafeCell::new(KernelStacks(
    [[0; KERNEL_STACK_SIZE]; MAX_PROCESSES],
)));

#[derive(Clone, Copy)]
struct Process {
    pid: usize,
    active: bool,
    stopped: bool,
    allows_disk_install: bool,
    uid: u32,
    gid: u32,
    is_admin: bool,
    username: [u8; imagineos_abi::ACCOUNT_NAME_SIZE],
    username_length: usize,
    frame: *mut TrapFrame,
    address_space: Option<AddressSpace>,
    name: [u8; imagineos_abi::PROCESS_NAME_SIZE],
    pending_signals: u64,
    signal_actions: [u64; 32],
    signal_restorer: u64,
    signal_frame: Option<TrapFrame>,
}

impl Process {
    const EMPTY: Self = Self {
        pid: 0,
        active: false,
        stopped: false,
        allows_disk_install: false,
        uid: 0,
        gid: 0,
        is_admin: true,
        username: {
            let mut name = [0; imagineos_abi::ACCOUNT_NAME_SIZE];
            name[0] = b'r';
            name[1] = b'o';
            name[2] = b'o';
            name[3] = b't';
            name
        },
        username_length: 4,
        frame: ptr::null_mut(),
        address_space: None,
        name: [0; imagineos_abi::PROCESS_NAME_SIZE],
        pending_signals: 0,
        signal_actions: [imagineos_abi::SIGNAL_DEFAULT; 32],
        signal_restorer: 0,
        signal_frame: None,
    };
}

struct Scheduler {
    processes: [Process; MAX_PROCESSES],
    count: usize,
    current: usize,
}

impl Scheduler {
    const fn new() -> Self {
        Self {
            processes: [Process::EMPTY; MAX_PROCESSES],
            count: 0,
            current: 0,
        }
    }
}

struct SharedScheduler(UnsafeCell<Scheduler>);
unsafe impl Sync for SharedScheduler {}
static SCHEDULER: SharedScheduler = SharedScheduler(UnsafeCell::new(Scheduler::new()));

#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub enum LoadError {
    InvalidElf(ElfError),
    OutOfMemory,
    InvalidMapping,
}

pub fn init(programs: &[(&[u8], usize)]) -> Result<(), LoadError> {
    debug_log("process: initializing scheduler\n");
    let scheduler = unsafe { &mut *SCHEDULER.0.get() };
    scheduler.processes = [Process::EMPTY; MAX_PROCESSES];
    scheduler.count = 0;
    scheduler.current = 0;
    unsafe {
        *DESCRIPTOR_TABLES.0.get() = [DescriptorTable::new(); MAX_PROCESSES];
    }

    for &(image, pid) in programs.iter().take(MAX_PROCESSES) {
        #[cfg(feature = "kernel-debug")]
        {
            crate::console_write("process: loading PID ");
            crate::console_write_number(pid as u64);
            crate::console_write(" ELF\n");
        }
        let slot = scheduler.count;
        let arguments: [&[u8]; 1] = [b"/sbin/init"];
        let process = load_elf(image, pid, slot, &arguments, &[], false, "/sbin/init")?;
        scheduler.processes[scheduler.count] = process;
        scheduler.count += 1;
    }
    if scheduler.count == 0 {
        return Err(LoadError::InvalidMapping);
    }
    Ok(())
}

fn load_elf(
    image: &[u8],
    pid: usize,
    slot: usize,
    arguments: &[&[u8]],
    environment: &[&[u8]],
    allows_disk_install: bool,
    name: &str,
) -> Result<Process, LoadError> {
    debug_log("elf: validating header\n");
    let elf = Elf64::parse(image).map_err(LoadError::InvalidElf)?;
    debug_log("elf: creating user page tables\n");
    let address_space = AddressSpace::new_user().ok_or(LoadError::OutOfMemory)?;

    for index in 0..elf.program_header_count() {
        let Some(segment) = elf.segment(index).map_err(LoadError::InvalidElf)? else {
            continue;
        };
        debug_log("elf: mapping LOAD segment\n");
        let segment_end = segment
            .virtual_address
            .checked_add(segment.memory_size)
            .ok_or(LoadError::InvalidMapping)?;
        let first_page = segment.virtual_address & !(PAGE_SIZE - 1);
        let last_page = segment_end
            .checked_add(PAGE_SIZE - 1)
            .ok_or(LoadError::InvalidMapping)?
            & !(PAGE_SIZE - 1);
        let mut page = first_page;
        while page < last_page {
            match address_space.map_user_page(page) {
                Ok(_) | Err(MapError::AlreadyMapped) => {}
                Err(_) => return Err(LoadError::OutOfMemory),
            }
            page += PAGE_SIZE;
        }

        zero_user_range(&address_space, segment.virtual_address, segment.memory_size)?;
        copy_to_user(
            &address_space,
            segment.virtual_address,
            elf.file_bytes(segment),
        )?;
    }

    let stack_start = USER_STACK_TOP
        .checked_sub(USER_STACK_SIZE)
        .ok_or(LoadError::InvalidMapping)?;
    let mut page = stack_start;
    while page < USER_STACK_TOP {
        address_space
            .map_user_page(page)
            .map_err(|_| LoadError::OutOfMemory)?;
        page += PAGE_SIZE;
    }
    let (user_stack, user_argv, user_envp) =
        prepare_user_stack(&address_space, arguments, environment)?;
    debug_log("elf: user stack mapped\n");
    if address_space.translate(elf.entry).is_none() {
        return Err(LoadError::InvalidMapping);
    }

    let stacks = unsafe { &mut *KERNEL_STACKS.0.get() };
    let kernel_stack_top = stacks.0[slot].as_mut_ptr() as u64 + KERNEL_STACK_SIZE as u64;
    let frame = (kernel_stack_top as usize - size_of::<TrapFrame>()) as *mut TrapFrame;
    unsafe {
        frame.write(TrapFrame {
            r15: 0,
            r14: 0,
            r13: 0,
            r12: 0,
            r11: 0,
            r10: 0,
            r9: 0,
            r8: 0,
            rbp: 0,
            rdx: environment.len() as u64,
            rcx: user_envp,
            rbx: 0,
            rax: 0,
            rip: elf.entry,
            cs: gdt::USER_CODE_SELECTOR,
            rflags: 0x2,
            rsp: user_stack,
            ss: gdt::USER_DATA_SELECTOR,
            rdi: arguments.len() as u64,
            rsi: user_argv,
        });
    }
    let mut process_name = [0; imagineos_abi::PROCESS_NAME_SIZE];
    let copied_name_length = name.len().min(process_name.len());
    process_name[..copied_name_length].copy_from_slice(&name.as_bytes()[..copied_name_length]);
    Ok(Process {
        pid,
        active: true,
        stopped: false,
        allows_disk_install,
        uid: 0,
        gid: 0,
        is_admin: true,
        username: {
            let mut username = [0; imagineos_abi::ACCOUNT_NAME_SIZE];
            username[..4].copy_from_slice(b"root");
            username
        },
        username_length: 4,
        frame,
        address_space: Some(address_space),
        name: process_name,
        pending_signals: 0,
        signal_actions: [imagineos_abi::SIGNAL_DEFAULT; 32],
        signal_restorer: 0,
        signal_frame: None,
    })
}

fn prepare_user_stack(
    space: &AddressSpace,
    arguments: &[&[u8]],
    environment: &[&[u8]],
) -> Result<(u64, u64, u64), LoadError> {
    if arguments.len() > MAX_EXEC_ARGS || environment.len() > MAX_EXEC_ENV {
        return Err(LoadError::InvalidMapping);
    }
    let mut stack_pointer = USER_STACK_TOP;
    let mut argument_pointers = [0u64; MAX_EXEC_ARGS];
    let mut environment_pointers = [0u64; MAX_EXEC_ENV];
    for (index, argument) in arguments.iter().enumerate() {
        stack_pointer = stack_pointer
            .checked_sub(argument.len() as u64 + 1)
            .ok_or(LoadError::InvalidMapping)?;
        copy_to_user(space, stack_pointer, argument)?;
        copy_to_user(space, stack_pointer + argument.len() as u64, &[0])?;
        argument_pointers[index] = stack_pointer;
    }
    for (index, entry) in environment.iter().enumerate() {
        stack_pointer = stack_pointer
            .checked_sub(entry.len() as u64 + 1)
            .ok_or(LoadError::InvalidMapping)?;
        copy_to_user(space, stack_pointer, entry)?;
        copy_to_user(space, stack_pointer + entry.len() as u64, &[0])?;
        environment_pointers[index] = stack_pointer;
    }

    let environment_vector_size = (environment.len() + 1)
        .checked_mul(size_of::<u64>())
        .ok_or(LoadError::InvalidMapping)? as u64;
    let environment_vector = stack_pointer
        .checked_sub(environment_vector_size)
        .ok_or(LoadError::InvalidMapping)?
        & !0xf;
    for (index, pointer) in environment_pointers
        .iter()
        .take(environment.len())
        .enumerate()
    {
        copy_to_user(
            space,
            environment_vector + (index * size_of::<u64>()) as u64,
            &pointer.to_le_bytes(),
        )?;
    }
    copy_to_user(
        space,
        environment_vector + (environment.len() * size_of::<u64>()) as u64,
        &0u64.to_le_bytes(),
    )?;

    let argument_vector_size = (arguments.len() + 1)
        .checked_mul(size_of::<u64>())
        .ok_or(LoadError::InvalidMapping)? as u64;
    stack_pointer = environment_vector
        .checked_sub(argument_vector_size)
        .ok_or(LoadError::InvalidMapping)?
        & !0xf;
    for (index, pointer) in argument_pointers.iter().take(arguments.len()).enumerate() {
        copy_to_user(
            space,
            stack_pointer + (index * size_of::<u64>()) as u64,
            &pointer.to_le_bytes(),
        )?;
    }
    copy_to_user(
        space,
        stack_pointer + (arguments.len() * size_of::<u64>()) as u64,
        &0u64.to_le_bytes(),
    )?;
    let entry_stack = stack_pointer
        .checked_sub(8)
        .ok_or(LoadError::InvalidMapping)?;
    Ok((entry_stack, stack_pointer, environment_vector))
}

fn zero_user_range(space: &AddressSpace, address: u64, size: u64) -> Result<(), LoadError> {
    let mut current = address;
    let end = address.checked_add(size).ok_or(LoadError::InvalidMapping)?;
    while current < end {
        let destination = space.translate(current).ok_or(LoadError::InvalidMapping)?;
        let available = (PAGE_SIZE - (current & (PAGE_SIZE - 1))).min(end - current);
        unsafe {
            ptr::write_bytes(destination, 0, available as usize);
        }
        current += available;
    }
    Ok(())
}

fn copy_to_user(space: &AddressSpace, address: u64, bytes: &[u8]) -> Result<(), LoadError> {
    let mut copied = 0usize;
    while copied < bytes.len() {
        let current = address
            .checked_add(copied as u64)
            .ok_or(LoadError::InvalidMapping)?;
        let destination = space.translate(current).ok_or(LoadError::InvalidMapping)?;
        let available =
            ((PAGE_SIZE - (current & (PAGE_SIZE - 1))) as usize).min(bytes.len() - copied);
        unsafe {
            ptr::copy_nonoverlapping(bytes.as_ptr().add(copied), destination, available);
        }
        copied += available;
    }
    Ok(())
}

pub fn start() -> ! {
    let scheduler = unsafe { &mut *SCHEDULER.0.get() };
    let process = scheduler.processes[0];
    scheduler.current = 0;
    let address_space = process.address_space.expect("process address space");
    let kernel_stack_top =
        unsafe { (*KERNEL_STACKS.0.get()).0[0].as_ptr() as u64 + KERNEL_STACK_SIZE as u64 };
    gdt::set_kernel_stack(kernel_stack_top);
    paging::switch(address_space);
    unsafe {
        asm!(
            "mov rsp, {frame}",
            "mov ax, 0x33",
            "mov ds, ax",
            "mov es, ax",
            "pop r15", "pop r14", "pop r13", "pop r12", "pop r11",
            "pop r10", "pop r9", "pop r8", "pop rbp", "pop rdi",
            "pop rsi", "pop rdx", "pop rcx", "pop rbx", "pop rax",
            "iretq",
            frame = in(reg) process.frame,
            options(noreturn)
        );
    }
}

pub fn current_pid() -> usize {
    let scheduler = unsafe { &*SCHEDULER.0.get() };
    scheduler.processes[scheduler.current].pid
}

pub fn current_credentials() -> (u32, u32, bool) {
    let scheduler = unsafe { &*SCHEDULER.0.get() };
    let process = scheduler.processes[scheduler.current];
    (process.uid, process.gid, process.is_admin)
}

pub fn yield_current(frame: *mut TrapFrame) -> *mut TrapFrame {
    schedule(frame, false)
}

pub fn exit_current(frame: *mut TrapFrame) -> *mut TrapFrame {
    schedule(frame, true)
}

pub fn terminate_faulting_process() -> *mut TrapFrame {
    let scheduler = unsafe { &*SCHEDULER.0.get() };
    let frame = scheduler.processes[scheduler.current].frame;
    if frame.is_null() {
        crate::kernel_halt();
    }
    schedule(frame, true)
}

pub fn send_signal(pid: usize, signal: u64) -> i64 {
    if !valid_signal(signal) {
        return -22;
    }
    let scheduler = unsafe { &mut *SCHEDULER.0.get() };
    let sender_uid = scheduler.processes[scheduler.current].uid;
    let Some(process) = scheduler
        .processes
        .iter_mut()
        .find(|process| process.active && process.pid == pid)
    else {
        return -3;
    };
    if sender_uid != 0 && sender_uid != process.uid {
        return -1;
    }
    if process.pid == 1 {
        return -1;
    }
    process.pending_signals |= 1u64 << signal;
    if signal == imagineos_abi::SIGNAL_CONT || signal == imagineos_abi::SIGNAL_KILL {
        process.stopped = false;
    }
    0
}

pub fn set_signal_action(signal: u64, handler: u64, restorer: u64) -> i64 {
    if !valid_signal(signal)
        || signal == imagineos_abi::SIGNAL_KILL
        || signal == imagineos_abi::SIGNAL_STOP
        || (handler != imagineos_abi::SIGNAL_DEFAULT
            && handler != imagineos_abi::SIGNAL_IGNORE
            && (restorer == 0
                || with_current_space(|space| {
                    space.translate(handler).is_none() || space.translate(restorer).is_none()
                })))
    {
        return -22;
    }
    let scheduler = unsafe { &mut *SCHEDULER.0.get() };
    let process = &mut scheduler.processes[scheduler.current];
    process.signal_actions[signal as usize] = handler;
    if handler > imagineos_abi::SIGNAL_IGNORE {
        process.signal_restorer = restorer;
    }
    0
}

pub fn restore_signal_context(frame: *mut TrapFrame) -> bool {
    let scheduler = unsafe { &mut *SCHEDULER.0.get() };
    let process = &mut scheduler.processes[scheduler.current];
    let Some(saved) = process.signal_frame.take() else {
        return false;
    };
    unsafe {
        frame.write(saved);
    }
    true
}

pub fn deliver_pending_signal(frame: *mut TrapFrame) -> *mut TrapFrame {
    if frame.is_null() {
        return frame;
    }
    loop {
        let scheduler = unsafe { &mut *SCHEDULER.0.get() };
        let slot = scheduler.current;
        let process = &mut scheduler.processes[slot];
        if !process.active {
            return frame;
        }
        let Some(signal) = next_pending_signal(process.pending_signals) else {
            return frame;
        };
        if process.signal_frame.is_some()
            && signal != imagineos_abi::SIGNAL_KILL
            && signal != imagineos_abi::SIGNAL_STOP
        {
            return frame;
        }
        process.pending_signals &= !(1u64 << signal);
        let handler = process.signal_actions[signal as usize];
        match handler {
            imagineos_abi::SIGNAL_IGNORE
                if signal != imagineos_abi::SIGNAL_KILL && signal != imagineos_abi::SIGNAL_STOP =>
            {
                continue;
            }
            _ if signal == imagineos_abi::SIGNAL_KILL
                || (handler == imagineos_abi::SIGNAL_DEFAULT && is_terminating_signal(signal)) =>
            {
                return schedule(frame, true);
            }
            _ if signal == imagineos_abi::SIGNAL_STOP => {
                process.stopped = true;
                return schedule(frame, false);
            }
            _ if signal == imagineos_abi::SIGNAL_CONT
                && handler <= imagineos_abi::SIGNAL_IGNORE =>
            {
                continue;
            }
            _ if handler > imagineos_abi::SIGNAL_IGNORE => {
                let Some(space) = process.address_space else {
                    return schedule(frame, true);
                };
                let interrupted = unsafe { *frame };
                // Enter the handler with the SysV stack alignment and a return stub.
                let stack_top = interrupted.rsp & !0xf;
                let Some(stack_slot) = stack_top.checked_sub(8) else {
                    return schedule(frame, true);
                };
                let Some(destination) = space.translate(stack_slot) else {
                    return schedule(frame, true);
                };
                unsafe {
                    ptr::write_unaligned(destination.cast::<u64>(), process.signal_restorer);
                }
                process.signal_frame = Some(interrupted);
                unsafe {
                    (*frame).rip = handler;
                    (*frame).rdi = signal;
                    (*frame).rsp = stack_slot;
                }
                return frame;
            }
            _ => continue,
        }
    }
}

pub fn snapshot() -> ([imagineos_abi::ProcessInfo; MAX_PROCESSES], usize) {
    let scheduler = unsafe { &*SCHEDULER.0.get() };
    let mut result = [imagineos_abi::ProcessInfo::default(); MAX_PROCESSES];
    let mut count = 0;
    for process in scheduler.processes.iter().filter(|process| process.active) {
        result[count] = imagineos_abi::ProcessInfo {
            pid: process.pid as u64,
            state: if process.stopped {
                imagineos_abi::PROCESS_STOPPED
            } else {
                imagineos_abi::PROCESS_RUNNING
            },
            pending_signals: process.pending_signals as u32,
            name: process.name,
        };
        count += 1;
    }
    (result, count)
}

fn schedule(frame: *mut TrapFrame, exiting: bool) -> *mut TrapFrame {
    let scheduler = unsafe { &mut *SCHEDULER.0.get() };
    let current = scheduler.current;
    scheduler.processes[current].frame = frame;
    if exiting {
        scheduler.processes[current].active = false;
    }

    for distance in 1..=MAX_PROCESSES {
        let candidate = (current + distance) % MAX_PROCESSES;
        if scheduler.processes[candidate].active && !scheduler.processes[candidate].stopped {
            scheduler.current = candidate;
            let next = scheduler.processes[candidate];
            let address_space = next.address_space.expect("process address space");
            let kernel_stack_top = unsafe {
                (*KERNEL_STACKS.0.get()).0[candidate].as_ptr() as u64 + KERNEL_STACK_SIZE as u64
            };
            gdt::set_kernel_stack(kernel_stack_top);
            paging::switch(address_space);
            return next.frame;
        }
    }

    crate::kernel_halt()
}

pub fn spawn_current(
    path: &str,
    arguments: &[&[u8]],
    environment: &[&[u8]],
    frame: *mut TrapFrame,
) -> *mut TrapFrame {
    if !has_access(path, 1) {
        unsafe {
            (*frame).rax = (-13i64) as u64;
        }
        return frame;
    }
    let Some(image) = crate::ramfs::read(path) else {
        unsafe {
            (*frame).rax = (-2i64) as u64;
        }
        return frame;
    };

    let (slot, pid, parent_slot) = {
        let scheduler = unsafe { &*SCHEDULER.0.get() };
        let Some(slot) = scheduler
            .processes
            .iter()
            .position(|process| !process.active)
        else {
            unsafe {
                (*frame).rax = (-11i64) as u64;
            }
            return frame;
        };
        let pid = scheduler
            .processes
            .iter()
            .map(|process| process.pid)
            .max()
            .unwrap_or(0)
            + 1;
        (slot, pid, scheduler.current)
    };

    let process = match load_elf(
        image,
        pid,
        slot,
        arguments,
        environment,
        path == "/bin/distroinstall",
        path,
    ) {
        Ok(process) => process,
        Err(_) => {
            unsafe {
                (*frame).rax = (-8i64) as u64;
            }
            return frame;
        }
    };
    unsafe {
        let tables = &mut *DESCRIPTOR_TABLES.0.get();
        for index in 0..MAX_OPEN_FDS {
            tables[slot].0[index] = tables[parent_slot].0[index];
        }
    }
    let scheduler = unsafe { &mut *SCHEDULER.0.get() };
    let parent = scheduler.processes[parent_slot];
    let mut process = process;
    process.uid = parent.uid;
    process.gid = parent.gid;
    process.is_admin = parent.is_admin;
    process.username = parent.username;
    process.username_length = parent.username_length;
    scheduler.processes[slot] = process;
    scheduler.count = scheduler.count.max(slot + 1);
    unsafe {
        (*frame).rax = pid as u64;
    }
    schedule(frame, false)
}

fn has_access(path: &str, requested: u16) -> bool {
    let (uid, gid, _) = current_credentials();
    if uid == 0 {
        return true;
    }
    let parent = path.rsplit_once('/').map_or("", |(parent, _)| parent);
    let mut prefix = [0u8; imagineos_abi::MAX_MUTABLE_PATH];
    let mut length = 0usize;
    for component in parent.trim_matches('/').split('/') {
        if component.is_empty() {
            continue;
        }
        if length != 0 {
            prefix[length] = b'/';
            length += 1;
        }
        if length + component.len() > prefix.len() {
            return false;
        }
        prefix[length..length + component.len()].copy_from_slice(component.as_bytes());
        length += component.len();
        let directory = core::str::from_utf8(&prefix[..length]).unwrap_or("");
        let Some(metadata) = crate::ramfs::metadata(directory) else {
            return false;
        };
        if !metadata.is_directory
            || !crate::permissions::allows(metadata.mode, metadata.uid, metadata.gid, uid, gid, 1)
        {
            return false;
        }
    }
    let Some(metadata) = crate::ramfs::metadata(path) else {
        return false;
    };
    crate::permissions::allows(
        metadata.mode,
        metadata.uid,
        metadata.gid,
        uid,
        gid,
        requested,
    )
}

pub fn authenticate_current(username: &[u8], password: &[u8], target_uid: u32) -> i64 {
    let scheduler = unsafe { &mut *SCHEDULER.0.get() };
    let current = &mut scheduler.processes[scheduler.current];
    let is_self = username == &current.username[..current.username_length];
    if target_uid == 0 && is_self && current.uid != 0 {
        // sudo authenticates the caller, then elevates only an explicitly enabled account.
        if !current.is_admin || crate::accounts::authenticate(username, password).is_none() {
            return -1;
        }
        current.uid = 0;
        current.gid = 0;
        current.is_admin = true;
        current.username = [0; imagineos_abi::ACCOUNT_NAME_SIZE];
        current.username[..4].copy_from_slice(b"root");
        current.username_length = 4;
        return 0;
    }
    let Some(account) = crate::accounts::authenticate(username, password) else {
        return -1;
    };
    if target_uid != u32::MAX && account.uid != target_uid {
        return -1;
    }
    current.uid = account.uid;
    current.gid = account.gid;
    current.is_admin = account.administrator;
    current.username = account.username;
    current.username_length = account.username_length;
    0
}

pub fn current_identity() -> imagineos_abi::UserIdentity {
    let scheduler = unsafe { &*SCHEDULER.0.get() };
    let process = scheduler.processes[scheduler.current];
    let mut identity = imagineos_abi::UserIdentity {
        uid: process.uid,
        gid: process.gid,
        is_admin: u32::from(process.is_admin),
        username_length: process.username_length as u32,
        username: process.username,
        ..imagineos_abi::UserIdentity::default()
    };
    if let Some(hostname) = crate::ramfs::read("/etc/hostname") {
        let hostname = hostname.strip_suffix(b"\n").unwrap_or(hostname);
        let length = hostname.len().min(identity.hostname.len());
        identity.hostname[..length].copy_from_slice(&hostname[..length]);
        identity.hostname_length = length as u32;
    } else {
        identity.hostname[..9].copy_from_slice(b"imagineos");
        identity.hostname_length = 9;
    }
    identity
}

fn valid_signal(signal: u64) -> bool {
    matches!(
        signal,
        imagineos_abi::SIGNAL_HUP
            | imagineos_abi::SIGNAL_INT
            | imagineos_abi::SIGNAL_KILL
            | imagineos_abi::SIGNAL_SEGV
            | imagineos_abi::SIGNAL_TERM
            | imagineos_abi::SIGNAL_CONT
            | imagineos_abi::SIGNAL_STOP
    )
}

fn next_pending_signal(pending: u64) -> Option<u64> {
    [
        imagineos_abi::SIGNAL_KILL,
        imagineos_abi::SIGNAL_STOP,
        imagineos_abi::SIGNAL_CONT,
        imagineos_abi::SIGNAL_HUP,
        imagineos_abi::SIGNAL_INT,
        imagineos_abi::SIGNAL_SEGV,
        imagineos_abi::SIGNAL_TERM,
    ]
    .into_iter()
    .find(|signal| pending & (1u64 << signal) != 0)
}

fn is_terminating_signal(signal: u64) -> bool {
    matches!(
        signal,
        imagineos_abi::SIGNAL_HUP
            | imagineos_abi::SIGNAL_INT
            | imagineos_abi::SIGNAL_SEGV
            | imagineos_abi::SIGNAL_TERM
    )
}

#[cfg(test)]
mod tests {
    use super::{is_terminating_signal, next_pending_signal, valid_signal};
    use imagineos_abi::{
        SIGNAL_HUP, SIGNAL_INT, SIGNAL_KILL, SIGNAL_SEGV, SIGNAL_STOP, SIGNAL_TERM,
    };

    #[test]
    fn accepts_only_signals_implemented_by_the_kernel() {
        for signal in [
            SIGNAL_HUP,
            SIGNAL_INT,
            SIGNAL_KILL,
            SIGNAL_SEGV,
            SIGNAL_TERM,
            imagineos_abi::SIGNAL_CONT,
            SIGNAL_STOP,
        ] {
            assert!(valid_signal(signal));
        }
        assert!(!valid_signal(0));
        assert!(!valid_signal(3));
    }

    #[test]
    fn uncatchable_termination_signals_have_priority_when_queued() {
        let pending = (1u64 << SIGNAL_TERM)
            | (1u64 << SIGNAL_STOP)
            | (1u64 << SIGNAL_KILL)
            | (1u64 << SIGNAL_INT);
        assert_eq!(next_pending_signal(pending), Some(SIGNAL_KILL));
        assert_eq!(next_pending_signal(1u64 << SIGNAL_STOP), Some(SIGNAL_STOP));
    }

    #[test]
    fn termination_defaults_match_the_supported_signal_contract() {
        assert!(is_terminating_signal(SIGNAL_HUP));
        assert!(is_terminating_signal(SIGNAL_INT));
        assert!(is_terminating_signal(SIGNAL_SEGV));
        assert!(is_terminating_signal(SIGNAL_TERM));
        assert!(!is_terminating_signal(SIGNAL_STOP));
        assert!(!is_terminating_signal(imagineos_abi::SIGNAL_CONT));
    }
}

pub fn can_install_to_disk() -> bool {
    let scheduler = unsafe { &*SCHEDULER.0.get() };
    scheduler.processes[scheduler.current].allows_disk_install
}

pub fn descriptor(fd: usize) -> Option<Descriptor> {
    let scheduler = unsafe { &*SCHEDULER.0.get() };
    (unsafe { &*DESCRIPTOR_TABLES.0.get() })[scheduler.current]
        .0
        .get(fd)
        .copied()
        .filter(|descriptor| descriptor.kind != DescriptorKind::Closed)
}

pub fn allocate_descriptor(descriptor: Descriptor) -> Option<usize> {
    let scheduler = unsafe { &mut *SCHEDULER.0.get() };
    (unsafe { &mut *DESCRIPTOR_TABLES.0.get() })[scheduler.current].allocate(descriptor)
}

pub fn update_descriptor(fd: usize, descriptor: Descriptor) -> bool {
    let scheduler = unsafe { &mut *SCHEDULER.0.get() };
    let Some(slot) = (unsafe { &mut *DESCRIPTOR_TABLES.0.get() })[scheduler.current]
        .0
        .get_mut(fd)
    else {
        return false;
    };
    if slot.kind == DescriptorKind::Closed {
        return false;
    }
    *slot = descriptor;
    true
}

pub fn close_descriptor(fd: usize) -> bool {
    let scheduler = unsafe { &mut *SCHEDULER.0.get() };
    let Some(slot) = (unsafe { &mut *DESCRIPTOR_TABLES.0.get() })[scheduler.current]
        .0
        .get_mut(fd)
    else {
        return false;
    };
    if slot.kind == DescriptorKind::Closed {
        return false;
    }
    *slot = Descriptor::CLOSED;
    true
}

pub fn with_current_space<T>(operation: impl FnOnce(AddressSpace) -> T) -> T {
    let scheduler = unsafe { &*SCHEDULER.0.get() };
    operation(
        scheduler.processes[scheduler.current]
            .address_space
            .expect("process address space"),
    )
}

pub fn copy_from_current_user(address: u64, output: &mut [u8]) -> bool {
    with_current_space(|space| {
        let mut copied = 0usize;
        while copied < output.len() {
            let Some(current) = address.checked_add(copied as u64) else {
                return false;
            };
            let Some(source) = space.translate(current) else {
                return false;
            };
            let available =
                ((PAGE_SIZE - (current & (PAGE_SIZE - 1))) as usize).min(output.len() - copied);
            unsafe {
                ptr::copy_nonoverlapping(source, output.as_mut_ptr().add(copied), available);
            }
            copied += available;
        }
        true
    })
}

pub fn copy_to_current_user(address: u64, input: &[u8]) -> bool {
    with_current_space(|space| {
        let mut copied = 0usize;
        while copied < input.len() {
            let Some(current) = address.checked_add(copied as u64) else {
                return false;
            };
            let Some(destination) = space.translate(current) else {
                return false;
            };
            let available =
                ((PAGE_SIZE - (current & (PAGE_SIZE - 1))) as usize).min(input.len() - copied);
            unsafe {
                ptr::copy_nonoverlapping(input.as_ptr().add(copied), destination, available);
            }
            copied += available;
        }
        true
    })
}

fn debug_log(message: &str) {
    #[cfg(feature = "kernel-debug")]
    crate::console_write(message);
    #[cfg(not(feature = "kernel-debug"))]
    let _ = message;
}

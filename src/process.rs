use core::arch::asm;
use core::cell::UnsafeCell;
use core::mem::size_of;
use core::ptr;

use crate::elf::{Elf64, Error as ElfError};
use crate::gdt;
use crate::paging::{self, AddressSpace, MapError};
use crate::syscall::TrapFrame;

const MAX_PROCESSES: usize = 4;
const KERNEL_STACK_SIZE: usize = 16 * 1024;
const USER_STACK_SIZE: u64 = 8 * 4096;
pub const USER_STACK_TOP: u64 = 0x0000_7fff_ffff_0000;
const PAGE_SIZE: u64 = 4096;

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
    frame: *mut TrapFrame,
    address_space: Option<AddressSpace>,
}

impl Process {
    const EMPTY: Self = Self {
        pid: 0,
        active: false,
        frame: ptr::null_mut(),
        address_space: None,
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
    crate::console_write("process: initializing scheduler\n");
    let scheduler = unsafe { &mut *SCHEDULER.0.get() };
    scheduler.processes = [Process::EMPTY; MAX_PROCESSES];
    scheduler.count = 0;
    scheduler.current = 0;

    for &(image, pid) in programs.iter().take(MAX_PROCESSES) {
        crate::console_write("process: loading PID ");
        crate::console_write_number(pid as u64);
        crate::console_write(" ELF\n");
        let process = load_elf(image, pid)?;
        scheduler.processes[scheduler.count] = process;
        scheduler.count += 1;
    }
    if scheduler.count == 0 {
        return Err(LoadError::InvalidMapping);
    }
    Ok(())
}

fn load_elf(image: &[u8], pid: usize) -> Result<Process, LoadError> {
    crate::console_write("elf: validating header\n");
    let elf = Elf64::parse(image).map_err(LoadError::InvalidElf)?;
    crate::console_write("elf: creating user page tables\n");
    let address_space = AddressSpace::new_user().ok_or(LoadError::OutOfMemory)?;

    for index in 0..elf.program_header_count() {
        let Some(segment) = elf.segment(index).map_err(LoadError::InvalidElf)? else {
            continue;
        };
        crate::console_write("elf: mapping LOAD segment\n");
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
    crate::console_write("elf: user stack mapped\n");
    if address_space.translate(elf.entry).is_none() {
        return Err(LoadError::InvalidMapping);
    }

    let stacks = unsafe { &mut *KERNEL_STACKS.0.get() };
    let kernel_stack_top = stacks.0[pid].as_mut_ptr() as u64 + KERNEL_STACK_SIZE as u64;
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
            rdi: 0,
            rsi: 0,
            rdx: 0,
            rcx: 0,
            rbx: 0,
            rax: 0,
            rip: elf.entry,
            cs: gdt::USER_CODE_SELECTOR,
            rflags: 0x2,
            rsp: USER_STACK_TOP - 16,
            ss: gdt::USER_DATA_SELECTOR,
        });
    }
    Ok(Process {
        pid,
        active: true,
        frame,
        address_space: Some(address_space),
    })
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
    let kernel_stack_top = unsafe {
        (*KERNEL_STACKS.0.get()).0[process.pid].as_ptr() as u64 + KERNEL_STACK_SIZE as u64
    };
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

pub fn yield_current(frame: *mut TrapFrame) -> *mut TrapFrame {
    schedule(frame, false)
}

pub fn exit_current(frame: *mut TrapFrame) -> *mut TrapFrame {
    schedule(frame, true)
}

fn schedule(frame: *mut TrapFrame, exiting: bool) -> *mut TrapFrame {
    let scheduler = unsafe { &mut *SCHEDULER.0.get() };
    let current = scheduler.current;
    scheduler.processes[current].frame = frame;
    if exiting {
        scheduler.processes[current].active = false;
    }

    for distance in 1..=scheduler.count {
        let candidate = (current + distance) % scheduler.count;
        if scheduler.processes[candidate].active {
            scheduler.current = candidate;
            let next = scheduler.processes[candidate];
            let address_space = next.address_space.expect("process address space");
            let kernel_stack_top = unsafe {
                (*KERNEL_STACKS.0.get()).0[next.pid].as_ptr() as u64 + KERNEL_STACK_SIZE as u64
            };
            gdt::set_kernel_stack(kernel_stack_top);
            paging::switch(address_space);
            return next.frame;
        }
    }

    crate::kernel_halt()
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

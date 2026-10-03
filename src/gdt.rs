use core::arch::asm;
use core::cell::UnsafeCell;
use core::mem::size_of;

const STACK_SIZE: usize = 16 * 1024;
const KERNEL_CODE_SELECTOR: u16 = 0x08;
const KERNEL_DATA_SELECTOR: u16 = 0x10;
const TSS_SELECTOR: u16 = 0x18;

#[repr(C, packed)]
struct TaskStateSegment {
    reserved_0: u32,
    privilege_stack: [u64; 3],
    reserved_1: u64,
    interrupt_stack: [u64; 7],
    reserved_2: u64,
    reserved_3: u16,
    io_map_base: u16,
}

impl TaskStateSegment {
    const fn new() -> Self {
        Self {
            reserved_0: 0,
            privilege_stack: [0; 3],
            reserved_1: 0,
            interrupt_stack: [0; 7],
            reserved_2: 0,
            reserved_3: 0,
            io_map_base: size_of::<Self>() as u16,
        }
    }
}

#[repr(C, align(16))]
struct Tables {
    gdt: [u64; 5],
    tss: TaskStateSegment,
    stack: [u8; STACK_SIZE],
}

struct SharedTables(UnsafeCell<Tables>);
unsafe impl Sync for SharedTables {}

static TABLES: SharedTables = SharedTables(UnsafeCell::new(Tables {
    gdt: [0; 5],
    tss: TaskStateSegment::new(),
    stack: [0; STACK_SIZE],
}));

#[repr(C, packed)]
struct DescriptorTablePointer {
    limit: u16,
    base: u64,
}

pub fn init() {
    let tables = unsafe { &mut *TABLES.0.get() };
    let stack_top = tables.stack.as_ptr() as u64 + STACK_SIZE as u64;
    tables.tss.privilege_stack[0] = stack_top;
    let tss_base = &tables.tss as *const TaskStateSegment as u64;
    let tss_limit = (size_of::<TaskStateSegment>() - 1) as u64;

    tables.gdt[0] = 0;
    tables.gdt[1] = 0x00af_9a00_0000_ffff;
    tables.gdt[2] = 0x00cf_9200_0000_ffff;
    tables.gdt[3] = (tss_limit & 0xffff)
        | ((tss_base & 0x00ff_ffff) << 16)
        | (0x89 << 40)
        | ((tss_limit & 0x000f_0000) << 32)
        | ((tss_base & 0xff00_0000) << 32);
    tables.gdt[4] = tss_base >> 32;

    let pointer = DescriptorTablePointer {
        limit: (size_of::<[u64; 5]>() - 1) as u16,
        base: tables.gdt.as_ptr() as u64,
    };
    unsafe {
        asm!(
            "lgdt [{pointer}]",
            "push {code_selector}",
            "lea rax, [rip + 2f]",
            "push rax",
            "retfq",
            "2:",
            "mov ax, {data_selector}",
            "mov ds, ax",
            "mov es, ax",
            "mov ss, ax",
            "mov ax, {tss_selector}",
            "ltr ax",
            pointer = in(reg) &pointer,
            code_selector = const KERNEL_CODE_SELECTOR,
            data_selector = const KERNEL_DATA_SELECTOR,
            tss_selector = const TSS_SELECTOR,
            out("rax") _,
        );
    }
}

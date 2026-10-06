use crate::memory;
use core::arch::asm;
use core::ptr;

const PAGE_SIZE: u64 = 4096;
const ENTRY_PRESENT: u64 = 1;
const ENTRY_WRITABLE: u64 = 1 << 1;
const ENTRY_USER: u64 = 1 << 2;
const ENTRY_HUGE: u64 = 1 << 7;
const ADDRESS_MASK: u64 = 0x000f_ffff_ffff_f000;
const USER_MIN: u64 = 0x400000;
const USER_MAX: u64 = 0x0000_8000_0000_0000;

#[derive(Clone, Copy)]
pub struct AddressSpace {
    root_physical: u64,
}

#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub enum MapError {
    InvalidAddress,
    OutOfMemory,
    AlreadyMapped,
}

impl AddressSpace {
    pub fn new_user() -> Option<Self> {
        let current_root = read_cr3() & ADDRESS_MASK;
        let root = memory::allocate_frame()?;
        unsafe {
            ptr::write_bytes(root.virtual_address, 0, PAGE_SIZE as usize);
            let source = memory::phys_to_virt(current_root)?.cast::<u64>();
            let destination = root.virtual_address.cast::<u64>();
            ptr::copy_nonoverlapping(source.add(256), destination.add(256), 256);
        }
        Some(Self {
            root_physical: root.physical_address,
        })
    }

    pub fn map_user_page(&self, virtual_address: u64) -> Result<*mut u8, MapError> {
        if !(USER_MIN..USER_MAX).contains(&virtual_address) {
            return Err(MapError::InvalidAddress);
        }
        let page_address = virtual_address & !(PAGE_SIZE - 1);
        let indices = [
            ((page_address >> 39) & 0x1ff) as usize,
            ((page_address >> 30) & 0x1ff) as usize,
            ((page_address >> 21) & 0x1ff) as usize,
            ((page_address >> 12) & 0x1ff) as usize,
        ];
        let mut table_physical = self.root_physical;
        for index in indices.iter().take(3) {
            let table = table_pointer(table_physical).ok_or(MapError::OutOfMemory)?;
            let entry = unsafe { &mut *table.add(*index) };
            if *entry & ENTRY_PRESENT == 0 {
                let frame = memory::allocate_frame().ok_or(MapError::OutOfMemory)?;
                unsafe {
                    ptr::write_bytes(frame.virtual_address, 0, PAGE_SIZE as usize);
                }
                *entry = frame.physical_address | ENTRY_PRESENT | ENTRY_WRITABLE | ENTRY_USER;
            } else {
                *entry |= ENTRY_WRITABLE | ENTRY_USER;
            }
            table_physical = *entry & ADDRESS_MASK;
        }

        let table = table_pointer(table_physical).ok_or(MapError::OutOfMemory)?;
        let entry = unsafe { &mut *table.add(indices[3]) };
        if *entry & ENTRY_PRESENT != 0 {
            return Err(MapError::AlreadyMapped);
        }
        let frame = memory::allocate_frame().ok_or(MapError::OutOfMemory)?;
        unsafe {
            ptr::write_bytes(frame.virtual_address, 0, PAGE_SIZE as usize);
        }
        *entry = frame.physical_address | ENTRY_PRESENT | ENTRY_WRITABLE | ENTRY_USER;
        Ok(frame.virtual_address)
    }

    pub fn translate(&self, virtual_address: u64) -> Option<*mut u8> {
        let indices = [
            ((virtual_address >> 39) & 0x1ff) as usize,
            ((virtual_address >> 30) & 0x1ff) as usize,
            ((virtual_address >> 21) & 0x1ff) as usize,
            ((virtual_address >> 12) & 0x1ff) as usize,
        ];
        let mut table_physical = self.root_physical;
        for index in indices.iter().take(3) {
            let table = table_pointer(table_physical)?;
            let entry = unsafe { *table.add(*index) };
            if entry & ENTRY_PRESENT == 0 || entry & ENTRY_USER == 0 {
                return None;
            }
            table_physical = entry & ADDRESS_MASK;
        }
        let table = table_pointer(table_physical)?;
        let entry = unsafe { *table.add(indices[3]) };
        if entry & ENTRY_PRESENT == 0 || entry & ENTRY_USER == 0 {
            return None;
        }
        let physical = (entry & ADDRESS_MASK) | (virtual_address & (PAGE_SIZE - 1));
        memory::phys_to_virt(physical)
    }
}

pub fn switch(space: AddressSpace) {
    unsafe {
        asm!("mov cr3, {}", in(reg) space.root_physical, options(nostack, preserves_flags));
    }
}

pub fn map_hhdm_range(hhdm_offset: u64, physical_address: u64, length: usize) -> bool {
    let Some(end) = physical_address.checked_add(length as u64) else {
        return false;
    };
    if length == 0 {
        return true;
    }

    let mut physical = physical_address & !(PAGE_SIZE - 1);
    let end = end.saturating_add(PAGE_SIZE - 1) & !(PAGE_SIZE - 1);
    while physical < end {
        let Some(virtual_address) = hhdm_offset.checked_add(physical) else {
            return false;
        };
        if !map_hhdm_page(virtual_address, physical) {
            return false;
        }
        physical = physical.saturating_add(PAGE_SIZE);
    }
    true
}

fn map_hhdm_page(virtual_address: u64, physical_address: u64) -> bool {
    let indices = [
        ((virtual_address >> 39) & 0x1ff) as usize,
        ((virtual_address >> 30) & 0x1ff) as usize,
        ((virtual_address >> 21) & 0x1ff) as usize,
        ((virtual_address >> 12) & 0x1ff) as usize,
    ];
    let mut table_physical = read_cr3() & ADDRESS_MASK;
    for (level, index) in indices.iter().take(3).enumerate() {
        let Some(table) = table_pointer(table_physical) else {
            return false;
        };
        let entry = unsafe { &mut *table.add(*index) };
        if *entry & ENTRY_PRESENT == 0 {
            let Some(frame) = memory::allocate_frame() else {
                return false;
            };
            unsafe {
                ptr::write_bytes(frame.virtual_address, 0, PAGE_SIZE as usize);
            }
            *entry = frame.physical_address | ENTRY_PRESENT | ENTRY_WRITABLE;
        } else if level > 0 && *entry & ENTRY_HUGE != 0 {
            return true;
        }
        table_physical = *entry & ADDRESS_MASK;
    }

    let Some(table) = table_pointer(table_physical) else {
        return false;
    };
    let entry = unsafe { &mut *table.add(indices[3]) };
    if *entry & ENTRY_PRESENT != 0 {
        return *entry & ADDRESS_MASK == physical_address;
    }
    *entry = physical_address | ENTRY_PRESENT | ENTRY_WRITABLE;
    unsafe {
        asm!("invlpg [{}]", in(reg) virtual_address, options(nostack, preserves_flags));
    }
    true
}

fn read_cr3() -> u64 {
    let value: u64;
    unsafe {
        asm!("mov {}, cr3", out(reg) value, options(nomem, nostack, preserves_flags));
    }
    value
}

fn table_pointer(physical: u64) -> Option<*mut u64> {
    Some(memory::phys_to_virt(physical)?.cast())
}

use core::arch::asm;
use core::cell::UnsafeCell;
use core::sync::atomic::{AtomicBool, Ordering};

use crate::block::{BlockDevice, BlockError};

const ACPI_HEADER_SIZE: usize = 36;
const ACPI_TABLE_MAX_SIZE: usize = 1024 * 1024;
const ACPI_SCI_ENABLED: u16 = 1;
const ACPI_SLEEP_ENABLE: u16 = 1 << 13;
const ACPI_ENABLE_POLL_LIMIT: usize = 1_000_000;

struct ShutdownMailbox(AtomicBool);

impl ShutdownMailbox {
    const fn new() -> Self {
        Self(AtomicBool::new(false))
    }

    fn send(&self) {
        self.0.store(true, Ordering::Release);
    }

    fn receive(&self) -> bool {
        self.0.swap(false, Ordering::AcqRel)
    }
}

static SHUTDOWN_MAILBOX: ShutdownMailbox = ShutdownMailbox::new();

#[derive(Clone, Copy)]
struct AcpiPowerRegisters {
    pm1a_control: u16,
    pm1b_control: Option<u16>,
    sleep_type_a: u16,
    sleep_type_b: u16,
    smi_command: u16,
    acpi_enable: u8,
}

struct SharedAcpiPower(UnsafeCell<Option<AcpiPowerRegisters>>);
unsafe impl Sync for SharedAcpiPower {}
static ACPI_POWER: SharedAcpiPower = SharedAcpiPower(UnsafeCell::new(None));

pub fn request_shutdown() {
    SHUTDOWN_MAILBOX.send();
}

pub fn take_shutdown_request() -> bool {
    SHUTDOWN_MAILBOX.receive()
}

pub fn init(rsdp_address: u64, hhdm_offset: u64) {
    if rsdp_address == 0 {
        crate::console_write("ACPI RSDP was not provided by Limine\n");
        unsafe {
            *ACPI_POWER.0.get() = None;
        }
        return;
    }
    let registers = unsafe { discover_acpi_power_registers(rsdp_address, hhdm_offset) };
    unsafe {
        *ACPI_POWER.0.get() = registers;
    }
    if registers.is_some() {
        crate::console_write("ACPI S5 poweroff support ready\n");
    } else {
        crate::console_write("ACPI poweroff tables unavailable\n");
    }
}

pub fn power_off() -> Result<(), BlockError> {
    if crate::ata::is_ready() {
        crate::ata::PrimaryMaster.flush()?;
    }

    let Some(registers) = (unsafe { *ACPI_POWER.0.get() }) else {
        return Err(BlockError::NotReady);
    };
    crate::console_write("Sync complete; requesting ACPI poweroff\n");
    unsafe {
        if read_word(registers.pm1a_control) & ACPI_SCI_ENABLED == 0 {
            if registers.smi_command == 0 || registers.acpi_enable == 0 {
                return Err(BlockError::DeviceError);
            }
            out_byte(registers.smi_command, registers.acpi_enable);
            let mut acpi_enabled = false;
            for _ in 0..ACPI_ENABLE_POLL_LIMIT {
                if read_word(registers.pm1a_control) & ACPI_SCI_ENABLED != 0 {
                    acpi_enabled = true;
                    break;
                }
                core::hint::spin_loop();
            }
            if !acpi_enabled {
                return Err(BlockError::Timeout);
            }
        }
        out_word(
            registers.pm1a_control,
            (registers.sleep_type_a << 10) | ACPI_SLEEP_ENABLE | ACPI_SCI_ENABLED,
        );
        if let Some(pm1b_control) = registers.pm1b_control {
            out_word(
                pm1b_control,
                (registers.sleep_type_b << 10) | ACPI_SLEEP_ENABLE | ACPI_SCI_ENABLED,
            );
        }
    }
    crate::console_write("ACPI S5 poweroff requested; halting CPU\n");
    loop {
        unsafe {
            asm!("cli; hlt", options(nomem, nostack));
        }
    }
}

unsafe fn out_word(port: u16, value: u16) {
    asm!(
        "out dx, ax",
        in("dx") port,
        in("ax") value,
        options(nomem, nostack, preserves_flags)
    );
}

unsafe fn out_byte(port: u16, value: u8) {
    asm!(
        "out dx, al",
        in("dx") port,
        in("al") value,
        options(nomem, nostack, preserves_flags)
    );
}

unsafe fn read_word(port: u16) -> u16 {
    let value: u16;
    asm!(
        "in ax, dx",
        in("dx") port,
        out("ax") value,
        options(nomem, nostack, preserves_flags)
    );
    value
}

unsafe fn discover_acpi_power_registers(
    rsdp_address: u64,
    hhdm_offset: u64,
) -> Option<AcpiPowerRegisters> {
    let Some(rsdp) = physical_bytes(rsdp_address, hhdm_offset, 36) else {
        crate::console_write("ACPI RSDP address is not mapped\n");
        return None;
    };
    if &rsdp[..8] != b"RSD PTR " || !checksum_valid(&rsdp[..20]) {
        crate::console_write("ACPI RSDP signature or checksum is invalid\n");
        return None;
    }

    let revision = rsdp[15];
    let (root_address, root_signature) = if revision >= 2 {
        let length = read_u32(&rsdp, 20)? as usize;
        if !(36..=4096).contains(&length) {
            return None;
        }
        let extended = physical_bytes(rsdp_address, hhdm_offset, length)?;
        if !checksum_valid(&extended) {
            return None;
        }
        let xsdt = read_u64(&extended, 24)?;
        if xsdt != 0 {
            (xsdt, b"XSDT" as &[u8])
        } else {
            (read_u32(&extended, 16)? as u64, b"RSDT" as &[u8])
        }
    } else {
        (read_u32(&rsdp, 16)? as u64, b"RSDT" as &[u8])
    };
    let Some(root) = acpi_table(root_address, hhdm_offset) else {
        crate::console_write("ACPI root table is invalid or unmapped\n");
        return None;
    };
    if &root[..4] != root_signature {
        crate::console_write("ACPI root table signature does not match RSDP\n");
        return None;
    }
    let entry_size = if root_signature == b"XSDT" { 8 } else { 4 };
    if (root.len() - ACPI_HEADER_SIZE) % entry_size != 0 {
        return None;
    }
    for entry in root[ACPI_HEADER_SIZE..].chunks_exact(entry_size) {
        let address = if entry_size == 8 {
            read_u64(entry, 0)?
        } else {
            read_u32(entry, 0)? as u64
        };
        let Some(fadt) = acpi_table(address, hhdm_offset) else {
            continue;
        };
        if &fadt[..4] == b"FACP" {
            let registers = parse_fadt_power_registers(&fadt, hhdm_offset);
            if registers.is_none() {
                crate::console_write("ACPI FADT has no supported S5/PM1 configuration\n");
            }
            return registers;
        }
    }
    crate::console_write("ACPI root table contains no valid FADT\n");
    None
}

unsafe fn parse_fadt_power_registers(fadt: &[u8], hhdm_offset: u64) -> Option<AcpiPowerRegisters> {
    let dsdt_address = if fadt.len() >= 148 {
        read_u64(fadt, 140)?.max(read_u32(fadt, 40)? as u64)
    } else {
        read_u32(fadt, 40)? as u64
    };
    let Some(dsdt) = acpi_table(dsdt_address, hhdm_offset) else {
        crate::console_write("ACPI DSDT is invalid or unmapped\n");
        return None;
    };
    if &dsdt[..4] != b"DSDT" {
        crate::console_write("ACPI DSDT address has the wrong signature\n");
        return None;
    }
    let Some((sleep_type_a, sleep_type_b)) = parse_s5_sleep_types(&dsdt[ACPI_HEADER_SIZE..]) else {
        crate::console_write("ACPI DSDT does not expose a supported _S5_ package\n");
        return None;
    };

    let pm1a_legacy = read_u32(fadt, 64)? as u64;
    let pm1b_legacy = read_u32(fadt, 68)? as u64;
    let pm1a_control = if pm1a_legacy != 0 {
        io_port(pm1a_legacy)?
    } else {
        extended_io_port(fadt, 172)?
    };
    let pm1b_control = if pm1b_legacy != 0 {
        Some(io_port(pm1b_legacy)?)
    } else {
        extended_io_port(fadt, 184)
    };
    if pm1a_control == 0 {
        crate::console_write("ACPI FADT has no usable PM1a control register\n");
        return None;
    }
    let smi_command = read_u32(fadt, 48)?;
    let acpi_enable = fadt[52];

    Some(AcpiPowerRegisters {
        pm1a_control,
        pm1b_control,
        sleep_type_a,
        sleep_type_b,
        smi_command: u16::try_from(smi_command).ok()?,
        acpi_enable,
    })
}

unsafe fn acpi_table<'a>(address: u64, hhdm_offset: u64) -> Option<&'a [u8]> {
    let header = physical_bytes(address, hhdm_offset, ACPI_HEADER_SIZE)?;
    let length = read_u32(&header, 4)? as usize;
    if !(ACPI_HEADER_SIZE..=ACPI_TABLE_MAX_SIZE).contains(&length) {
        return None;
    }
    let table = physical_bytes(address, hhdm_offset, length)?;
    checksum_valid(&table).then_some(table)
}

unsafe fn physical_bytes<'a>(address: u64, hhdm_offset: u64, length: usize) -> Option<&'a [u8]> {
    if !crate::paging::map_hhdm_range(hhdm_offset, address, length) {
        return None;
    }
    let virtual_address = hhdm_offset.checked_add(address)?;
    let pointer = usize::try_from(virtual_address).ok()? as *const u8;
    Some(core::slice::from_raw_parts(pointer, length))
}

fn checksum_valid(bytes: &[u8]) -> bool {
    bytes.iter().fold(0u8, |sum, byte| sum.wrapping_add(*byte)) == 0
}

fn io_port(address: u64) -> Option<u16> {
    let port = u16::try_from(address).ok()?;
    (port != 0).then_some(port)
}

fn extended_io_port(fadt: &[u8], offset: usize) -> Option<u16> {
    if fadt.len() < offset.checked_add(12)? || fadt[offset] != 1 || fadt[offset + 1] < 16 {
        return None;
    }
    io_port(read_u64(fadt, offset + 4)?)
}

fn read_u32(bytes: &[u8], offset: usize) -> Option<u32> {
    Some(u32::from_le_bytes(
        bytes.get(offset..offset.checked_add(4)?)?.try_into().ok()?,
    ))
}

fn read_u64(bytes: &[u8], offset: usize) -> Option<u64> {
    Some(u64::from_le_bytes(
        bytes.get(offset..offset.checked_add(8)?)?.try_into().ok()?,
    ))
}

fn parse_s5_sleep_types(aml: &[u8]) -> Option<(u16, u16)> {
    for start in 0..aml.len().saturating_sub(4) {
        let name_length = if aml[start..].starts_with(&[0x08, 0x5c, b'_', b'S', b'5', b'_']) {
            6
        } else if aml[start..].starts_with(&[0x08, b'_', b'S', b'5', b'_']) {
            5
        } else {
            continue;
        };
        let package = start + name_length;
        if aml.get(package) != Some(&0x12) {
            continue;
        }
        let Some(package_bytes) = aml.get(package + 1..) else {
            continue;
        };
        let Some((_, length_bytes)) = aml_package_length(package_bytes) else {
            continue;
        };
        let elements = package + 1 + length_bytes;
        if aml.get(elements).map_or(true, |count| *count < 2) {
            continue;
        }
        let Some(first_element) = aml.get(elements + 1..) else {
            continue;
        };
        let Some((sleep_type_a, a_length)) = aml_integer(first_element) else {
            continue;
        };
        let Some(second_element) = aml.get(elements + 1 + a_length..) else {
            continue;
        };
        let Some((sleep_type_b, _)) = aml_integer(second_element) else {
            continue;
        };
        if sleep_type_a <= 7 && sleep_type_b <= 7 {
            return Some((sleep_type_a, sleep_type_b));
        }
    }
    None
}

fn aml_package_length(bytes: &[u8]) -> Option<(usize, usize)> {
    let lead = *bytes.first()?;
    let following = (lead >> 6) as usize;
    if following == 0 {
        return Some(((lead & 0x3f) as usize, 1));
    }
    if bytes.len() <= following {
        return None;
    }
    let mut length = (lead & 0x0f) as usize;
    for index in 0..following {
        length |= (bytes[index + 1] as usize) << (4 + index * 8);
    }
    Some((length, following + 1))
}

fn aml_integer(bytes: &[u8]) -> Option<(u16, usize)> {
    match *bytes.first()? {
        0x00 => Some((0, 1)),
        0x01 => Some((1, 1)),
        0x0a => Some((*bytes.get(1)? as u16, 2)),
        0x0b => Some((u16::from_le_bytes(bytes.get(1..3)?.try_into().ok()?), 3)),
        0x0c => {
            let value = read_u32(bytes, 1)?;
            Some((u16::try_from(value).ok()?, 5))
        }
        _ => None,
    }
}

#[cfg(test)]
mod tests {
    use super::{aml_integer, aml_package_length, parse_s5_sleep_types, ShutdownMailbox};

    #[test]
    fn reads_sleep_state_five_from_aml_package() {
        let aml = [
            0x08, 0x5c, b'_', b'S', b'5', b'_', 0x12, 0x06, 0x02, 0x0a, 0x05, 0x0a, 0x05,
        ];
        assert_eq!(parse_s5_sleep_types(&aml), Some((5, 5)));
    }

    #[test]
    fn reads_unqualified_sleep_state_five_from_aml_package() {
        let aml = [
            0x08, b'_', b'S', b'5', b'_', 0x12, 0x06, 0x02, 0x0a, 0x05, 0x0a, 0x05,
        ];
        assert_eq!(parse_s5_sleep_types(&aml), Some((5, 5)));
    }

    #[test]
    fn decodes_aml_package_lengths_and_integer_values() {
        assert_eq!(aml_package_length(&[0x3f]), Some((63, 1)));
        assert_eq!(aml_package_length(&[0x41, 0x02]), Some((33, 2)));
        assert_eq!(aml_integer(&[0x0b, 0x34, 0x12]), Some((0x1234, 3)));
        assert_eq!(aml_integer(&[0x0c, 0x34, 0x12, 0, 0]), Some((0x1234, 5)));
    }

    #[test]
    fn shutdown_request_is_received_once() {
        let mailbox = ShutdownMailbox::new();
        assert!(!mailbox.receive());
        mailbox.send();
        assert!(mailbox.receive());
        assert!(!mailbox.receive());
    }

    #[test]
    fn duplicate_requests_coalesce_into_one_pending_request() {
        let mailbox = ShutdownMailbox::new();
        mailbox.send();
        mailbox.send();
        assert!(mailbox.receive());
        assert!(!mailbox.receive());
    }
}

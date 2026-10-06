use core::arch::asm;
use core::sync::atomic::{AtomicU64, Ordering};

#[cfg(feature = "bootstrap")]
const PIT_FREQUENCY: u64 = 1_193_182;
#[cfg(feature = "bootstrap")]
const PIT_RELOAD: u64 = u16::MAX as u64;
#[cfg(feature = "bootstrap")]
const POLL_LIMIT: usize = 100_000_000;

static START_TSC: AtomicU64 = AtomicU64::new(0);
static TSC_FREQUENCY: AtomicU64 = AtomicU64::new(0);

#[cfg(feature = "bootstrap")]
pub fn init() -> (u64, u64) {
    let start = read_tsc();
    let frequency = calibrate_tsc().or_else(cpuid_tsc_frequency).unwrap_or(0);
    init_with(start, frequency);
    (start, frequency)
}

pub fn init_with(start: u64, frequency: u64) {
    START_TSC.store(start, Ordering::Relaxed);
    TSC_FREQUENCY.store(frequency, Ordering::Relaxed);
}

#[cfg(any(not(feature = "bootstrap"), test))]
pub fn format_elapsed(output: &mut [u8; 32]) -> &[u8] {
    let frequency = TSC_FREQUENCY.load(Ordering::Relaxed);
    if frequency == 0 {
        return b"[time?] ";
    }

    let cycles = read_tsc().saturating_sub(START_TSC.load(Ordering::Relaxed));
    let milliseconds = cycles / frequency * 1000 + (cycles % frequency) * 1000 / frequency;
    format_milliseconds(milliseconds, output)
}

#[cfg(any(not(feature = "bootstrap"), test))]
fn format_milliseconds(milliseconds: u64, output: &mut [u8; 32]) -> &[u8] {
    let seconds = milliseconds / 1000;
    let mut cursor = 0;
    output[cursor] = b'[';
    cursor += 1;
    cursor += write_decimal(&mut output[cursor..], seconds, 6);
    output[cursor] = b'.';
    cursor += 1;
    cursor += write_decimal(&mut output[cursor..], milliseconds % 1000, 3);
    output[cursor] = b']';
    output[cursor + 1] = b' ';
    &output[..cursor + 2]
}

#[cfg(feature = "bootstrap")]
fn calibrate_tsc() -> Option<u64> {
    let previous_control = unsafe { in_byte(0x61) };
    unsafe {
        out_byte(0x61, previous_control & !0x03);
        out_byte(0x43, 0xb0);
        out_byte(0x42, PIT_RELOAD as u8);
        out_byte(0x42, (PIT_RELOAD >> 8) as u8);
        out_byte(0x61, (previous_control & !0x02) | 0x01);
    }

    let mut remaining = POLL_LIMIT;
    while unsafe { in_byte(0x61) } & 0x20 != 0 && remaining != 0 {
        remaining -= 1;
        core::hint::spin_loop();
    }

    let start = read_tsc();
    while unsafe { in_byte(0x61) } & 0x20 == 0 && remaining != 0 {
        remaining -= 1;
        core::hint::spin_loop();
    }
    let end = read_tsc();
    unsafe {
        out_byte(0x61, previous_control);
    }

    if remaining == 0 || end <= start {
        return None;
    }
    Some((end - start) * PIT_FREQUENCY / PIT_RELOAD)
}

#[cfg(feature = "bootstrap")]
fn cpuid_tsc_frequency() -> Option<u64> {
    let maximum_leaf = core::arch::x86_64::__cpuid(0).eax;
    if maximum_leaf >= 0x15 {
        let leaf = core::arch::x86_64::__cpuid_count(0x15, 0);
        if leaf.eax != 0 && leaf.ebx != 0 && leaf.ecx != 0 {
            return Some((leaf.ecx as u64) * (leaf.ebx as u64) / (leaf.eax as u64));
        }
    }
    if maximum_leaf >= 0x16 {
        let nominal_mhz = core::arch::x86_64::__cpuid(0x16).eax;
        if nominal_mhz != 0 {
            return Some(nominal_mhz as u64 * 1_000_000);
        }
    }
    None
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

#[cfg(any(not(feature = "bootstrap"), test))]
fn write_decimal(output: &mut [u8], mut value: u64, minimum_digits: usize) -> usize {
    let mut digits = [0u8; 20];
    let mut start = digits.len();
    loop {
        start -= 1;
        digits[start] = b'0' + (value % 10) as u8;
        value /= 10;
        if value == 0 {
            break;
        }
    }
    while digits.len() - start < minimum_digits {
        start -= 1;
        digits[start] = b'0';
    }
    let length = digits.len() - start;
    output[..length].copy_from_slice(&digits[start..]);
    length
}

#[cfg(feature = "bootstrap")]
unsafe fn in_byte(port: u16) -> u8 {
    let value: u8;
    asm!(
        "in al, dx",
        in("dx") port,
        out("al") value,
        options(nomem, nostack, preserves_flags)
    );
    value
}

#[cfg(feature = "bootstrap")]
unsafe fn out_byte(port: u16, value: u8) {
    asm!(
        "out dx, al",
        in("dx") port,
        in("al") value,
        options(nomem, nostack, preserves_flags)
    );
}

#[cfg(test)]
mod tests {
    use super::{format_milliseconds, write_decimal};

    #[test]
    fn decimal_format_has_minimum_width_without_truncating_large_values() {
        let mut output = [0; 20];
        let length = write_decimal(&mut output, 42, 6);
        assert_eq!(&output[..length], b"000042");

        let length = write_decimal(&mut output, 1_234_567, 6);
        assert_eq!(&output[..length], b"1234567");
    }

    #[test]
    fn elapsed_timestamp_is_bracketed_and_includes_milliseconds() {
        let mut output = [0; 32];
        assert_eq!(format_milliseconds(42_007, &mut output), b"[000042.007] ");
        assert_eq!(
            format_milliseconds(1_234_567_890, &mut output),
            b"[1234567.890] "
        );
    }
}

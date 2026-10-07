#![no_std]
#![no_main]

fn user_main(
    mut args: imagineos_rt::Arguments<'_>,
    _env: imagineos_rt::Environment<'_>,
) -> imagineos_rt::imagineos::Result<()> {
    let mut line_limit = None;
    while let Some(argument) = args.next() {
        let argument = argument?;
        match argument {
            "-h" | "--help" => {
                imagineos_rt::println!(
                    "Usage: dmesg [-n LINES]\nShow the newest kernel log messages."
                )?;
                return Ok(());
            }
            "-n" | "--lines" => {
                let value = args
                    .next()
                    .ok_or(imagineos_rt::imagineos::Error::INVALID_ARGUMENT)??;
                line_limit = Some(parse_count(value)?);
            }
            _ => {
                imagineos_rt::imagineos::console::write_error(
                    b"dmesg: expected --help or -n LINES\n",
                )?;
                return Err(imagineos_rt::imagineos::Error::INVALID_ARGUMENT);
            }
        }
    }

    let mut buffer = [0u8; imagineos_rt::imagineos::abi::MAX_READ_BUFFER];
    let length = imagineos_rt::imagineos::kernel_log::read(&mut buffer)?;
    let start = line_limit.map_or(0, |count| last_lines(&buffer[..length], count));
    imagineos_rt::imagineos::console::write_all(&buffer[start..length])?;
    Ok(())
}

fn parse_count(value: &str) -> imagineos_rt::imagineos::Result<usize> {
    if value.is_empty() {
        return Err(imagineos_rt::imagineos::Error::INVALID_ARGUMENT);
    }
    value.bytes().try_fold(0usize, |number, digit| {
        if !digit.is_ascii_digit() {
            return Err(imagineos_rt::imagineos::Error::INVALID_ARGUMENT);
        }
        number
            .checked_mul(10)
            .and_then(|number| number.checked_add((digit - b'0') as usize))
            .ok_or(imagineos_rt::imagineos::Error::TOO_BIG)
    })
}

fn last_lines(bytes: &[u8], count: usize) -> usize {
    if count == 0 {
        return bytes.len();
    }
    let line_count = bytes.iter().filter(|byte| **byte == b'\n').count()
        + usize::from(bytes.last().is_some_and(|byte| *byte != b'\n'));
    let mut lines_to_skip = line_count.saturating_sub(count);
    for (index, byte) in bytes.iter().enumerate() {
        if *byte == b'\n' && lines_to_skip != 0 {
            lines_to_skip -= 1;
            if lines_to_skip == 0 {
                return index + 1;
            }
        }
    }
    0
}

imagineos_rt::main!(with_args user_main);

#[cfg(test)]
mod tests {
    use super::{last_lines, parse_count};

    #[test]
    fn parses_non_negative_line_counts() {
        assert_eq!(parse_count("42").unwrap(), 42);
        assert!(parse_count("").is_err());
        assert!(parse_count("-1").is_err());
    }

    #[test]
    fn selects_the_requested_tail_of_a_log() {
        let log = b"one\ntwo\nthree\n";
        assert_eq!(&log[last_lines(log, 2)..], b"two\nthree\n");
        assert_eq!(&log[last_lines(log, 0)..], b"");
        assert_eq!(&log[last_lines(log, 8)..], log);
        let unterminated = b"one\ntwo\nthree";
        assert_eq!(&unterminated[last_lines(unterminated, 2)..], b"two\nthree");
    }
}

#[path = "../dnu/dzimage.rs"]
#[allow(dead_code)]
mod dzimage;

use std::env;
use std::fs;
use std::process;

const HASH_BITS: usize = 16;
const HASH_SIZE: usize = 1 << HASH_BITS;

fn main() {
    if let Err(error) = run() {
        eprintln!("dzpack: {error}");
        process::exit(1);
    }
}

fn run() -> Result<(), Box<dyn std::error::Error>> {
    let mut arguments = env::args_os().skip(1);
    let input_path = arguments.next().ok_or("usage: dzpack INPUT OUTPUT")?;
    let output_path = arguments.next().ok_or("usage: dzpack INPUT OUTPUT")?;
    if arguments.next().is_some() {
        return Err("usage: dzpack INPUT OUTPUT".into());
    }

    let input = fs::read(input_path)?;
    if input.is_empty() || input.len() > dzimage::MAX_KERNEL_SIZE {
        return Err(format!("kernel size is outside the supported range: {}", input.len()).into());
    }
    let compressed = compress(&input);
    let mut image = Vec::with_capacity(dzimage::HEADER_SIZE + compressed.len());
    image.extend_from_slice(dzimage::MAGIC);
    image.extend_from_slice(&(input.len() as u64).to_le_bytes());
    image.extend_from_slice(&dzimage::crc32(&input).to_le_bytes());
    image.extend_from_slice(&[0; 4]);
    image.extend_from_slice(&compressed);
    fs::write(output_path, image)?;
    Ok(())
}

fn compress(input: &[u8]) -> Vec<u8> {
    let mut output = Vec::with_capacity(input.len());
    let mut table = vec![usize::MAX; HASH_SIZE];
    let mut anchor = 0usize;
    let mut cursor = 0usize;

    while cursor + 4 <= input.len() {
        let hash = hash_sequence(&input[cursor..cursor + 4]);
        let candidate = table[hash];
        table[hash] = cursor;
        if candidate == usize::MAX
            || cursor - candidate > u16::MAX as usize
            || input[candidate..candidate + 4] != input[cursor..cursor + 4]
        {
            cursor += 1;
            continue;
        }

        let mut match_end = cursor + 4;
        let mut candidate_end = candidate + 4;
        while match_end < input.len() && input[match_end] == input[candidate_end] {
            match_end += 1;
            candidate_end += 1;
        }
        emit_sequence(&mut output, &input[anchor..cursor], cursor - candidate, match_end - cursor);
        cursor = match_end;
        anchor = cursor;
        let mut update = cursor.saturating_sub(2);
        while update + 4 <= cursor {
            let hash = hash_sequence(&input[update..update + 4]);
            table[hash] = update;
            update += 1;
        }
    }

    emit_last_literals(&mut output, &input[anchor..]);
    output
}

fn hash_sequence(bytes: &[u8]) -> usize {
    let word = u32::from_le_bytes([bytes[0], bytes[1], bytes[2], bytes[3]]);
    ((word.wrapping_mul(2_654_435_761) >> (32 - HASH_BITS)) as usize) & (HASH_SIZE - 1)
}

fn emit_sequence(output: &mut Vec<u8>, literals: &[u8], offset: usize, match_length: usize) {
    let literal_nibble = literals.len().min(15);
    let match_nibble = (match_length - 4).min(15);
    let token_offset = output.len();
    output.push(((literal_nibble << 4) | match_nibble) as u8);
    if literals.len() >= 15 {
        emit_extended_length(output, literals.len() - 15);
    }
    output.extend_from_slice(literals);
    output.extend_from_slice(&(offset as u16).to_le_bytes());
    if match_length - 4 >= 15 {
        emit_extended_length(output, match_length - 4 - 15);
    }
    debug_assert!(token_offset < output.len());
}

fn emit_last_literals(output: &mut Vec<u8>, literals: &[u8]) {
    output.push((literals.len().min(15) << 4) as u8);
    if literals.len() >= 15 {
        emit_extended_length(output, literals.len() - 15);
    }
    output.extend_from_slice(literals);
}

fn emit_extended_length(output: &mut Vec<u8>, mut length: usize) {
    while length >= 255 {
        output.push(255);
        length -= 255;
    }
    output.push(length as u8);
}

#[cfg(test)]
mod tests {
    use super::compress;

    #[test]
    fn compresses_repeated_and_literal_data() {
        let input = b"Dreamcore Dreamcore Dreamcore and literal tail";
        let compressed = compress(input);
        assert!(compressed.len() < input.len());
        assert_round_trip(input, &compressed);
    }

    #[test]
    fn emits_a_valid_literal_only_block_for_short_inputs() {
        let compressed = compress(b"tiny");
        assert_eq!(compressed, [0x40, b't', b'i', b'n', b'y']);
        assert_round_trip(b"tiny", &compressed);
    }

    fn assert_round_trip(input: &[u8], compressed: &[u8]) {
        let mut image = Vec::new();
        image.extend_from_slice(crate::dzimage::MAGIC);
        image.extend_from_slice(&(input.len() as u64).to_le_bytes());
        image.extend_from_slice(&crate::dzimage::crc32(input).to_le_bytes());
        image.extend_from_slice(&[0; 4]);
        image.extend_from_slice(compressed);
        let mut output = vec![0; input.len()];
        crate::dzimage::unpack(&image, &mut output, |_| {}).unwrap();
        assert_eq!(output, input);
    }
}

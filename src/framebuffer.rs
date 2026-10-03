use core::cell::UnsafeCell;
use core::ptr;
use limine::framebuffer::Framebuffer;

struct Console {
    address: *mut u8,
    width: usize,
    height: usize,
    pitch: usize,
    red_shift: u8,
    green_shift: u8,
    blue_shift: u8,
    cursor_x: usize,
    cursor_y: usize,
    font: *const u8,
    font_size: usize,
    font_width: usize,
    font_height: usize,
    glyph_count: usize,
    glyph_bytes: usize,
    glyph_offset: usize,
    unicode_offset: usize,
    has_unicode_table: bool,
}

impl Console {
    const fn empty() -> Self {
        Self {
            address: ptr::null_mut(),
            width: 0,
            height: 0,
            pitch: 0,
            red_shift: 0,
            green_shift: 0,
            blue_shift: 0,
            cursor_x: 0,
            cursor_y: 0,
            font: ptr::null(),
            font_size: 0,
            font_width: 6,
            font_height: 8,
            glyph_count: 0,
            glyph_bytes: 0,
            glyph_offset: 0,
            unicode_offset: 0,
            has_unicode_table: false,
        }
    }
}

struct SharedConsole(UnsafeCell<Console>);
unsafe impl Sync for SharedConsole {}
static CONSOLE: SharedConsole = SharedConsole(UnsafeCell::new(Console::empty()));

pub fn init(framebuffer: Option<Framebuffer<'_>>) {
    let Some(framebuffer) = framebuffer else {
        return;
    };
    if framebuffer.bpp() != 32 || framebuffer.width() == 0 || framebuffer.height() == 0 {
        return;
    }
    let console = unsafe { &mut *CONSOLE.0.get() };
    console.address = framebuffer.addr();
    console.width = framebuffer.width() as usize;
    console.height = framebuffer.height() as usize;
    console.pitch = framebuffer.pitch() as usize;
    console.red_shift = framebuffer.red_mask_shift();
    console.green_shift = framebuffer.green_mask_shift();
    console.blue_shift = framebuffer.blue_mask_shift();
    clear();
}

pub fn load_font(bytes: &[u8]) -> bool {
    let console = unsafe { &mut *CONSOLE.0.get() };
    if console.address.is_null() {
        return false;
    }
    if bytes.len() >= 4 && bytes[0] == 0x36 && bytes[1] == 0x04 {
        let glyph_count: usize = if bytes[2] & 1 != 0 { 512 } else { 256 };
        let glyph_bytes = bytes[3] as usize;
        let total_bytes = glyph_count.checked_mul(glyph_bytes);
        if glyph_bytes > 0 && total_bytes.is_some_and(|size| size <= bytes.len() - 4) {
            console.font = bytes.as_ptr();
            console.font_size = bytes.len();
            console.font_width = 8;
            console.font_height = glyph_bytes;
            console.glyph_count = glyph_count;
            console.glyph_bytes = glyph_bytes;
            console.glyph_offset = 4;
            console.unicode_offset = 4 + glyph_count * glyph_bytes;
            console.has_unicode_table = bytes[2] & 2 != 0;
            return true;
        }
    }
    if bytes.len() >= 32 && read_u32(bytes, 0) == Some(0x864ab572) {
        let header_size = read_u32(bytes, 8).unwrap_or(0) as usize;
        let glyph_count = read_u32(bytes, 16).unwrap_or(0) as usize;
        let glyph_bytes = read_u32(bytes, 20).unwrap_or(0) as usize;
        let height = read_u32(bytes, 24).unwrap_or(0) as usize;
        let width = read_u32(bytes, 28).unwrap_or(0) as usize;
        let glyph_data_size = glyph_count.checked_mul(glyph_bytes);
        if header_size >= 32
            && header_size <= bytes.len()
            && glyph_count > 0
            && glyph_bytes > 0
            && width > 0
            && height > 0
            && glyph_data_size.is_some_and(|size| {
                header_size
                    .checked_add(size)
                    .is_some_and(|end| end <= bytes.len())
            })
        {
            console.font = bytes.as_ptr();
            console.font_size = bytes.len();
            console.font_width = width;
            console.font_height = height;
            console.glyph_count = glyph_count;
            console.glyph_bytes = glyph_bytes;
            console.glyph_offset = header_size;
            console.unicode_offset = header_size + glyph_data_size.unwrap_or(0);
            console.has_unicode_table = read_u32(bytes, 12).unwrap_or(0) & 1 != 0;
            return true;
        }
    }
    false
}

pub fn write_str(text: &str) {
    for character in text.chars() {
        write_char(character);
    }
}

pub fn write_char(character: char) {
    let console = unsafe { &mut *CONSOLE.0.get() };
    if console.address.is_null() {
        return;
    }
    match character {
        '\n' => {
            console.cursor_x = 0;
            console.cursor_y += console.font_height;
        }
        '\r' => console.cursor_x = 0,
        '\u{8}' => console.cursor_x = console.cursor_x.saturating_sub(console.font_width),
        '\t' => console.cursor_x += console.font_width * 4,
        _ => draw_glyph(console, character),
    }
    if console.cursor_x + console.font_width > console.width {
        console.cursor_x = 0;
        console.cursor_y += console.font_height;
    }
    if console.cursor_y + console.font_height > console.height {
        clear();
        console.cursor_x = 0;
        console.cursor_y = 0;
    }
}

pub fn clear() {
    let console = unsafe { &mut *CONSOLE.0.get() };
    if console.address.is_null() {
        return;
    }
    for y in 0..console.height {
        for x in 0..console.width {
            pixel(console, x, y, 0x101820);
        }
    }
    console.cursor_x = 0;
    console.cursor_y = 0;
}

fn draw_glyph(console: &mut Console, character: char) {
    if console.font.is_null() {
        draw_builtin(console, character);
        console.cursor_x += console.font_width;
        return;
    }
    let font = unsafe { core::slice::from_raw_parts(console.font, console.font_size) };
    let glyph = glyph_index(console, font, character).unwrap_or(b'?' as usize);
    if glyph >= console.glyph_count || console.font_width > 32 || console.font_height > 64 {
        return;
    }
    let start = console.glyph_offset + glyph * console.glyph_bytes;
    let row_bytes = (console.font_width + 7) / 8;
    for row in 0..console.font_height {
        for column in 0..console.font_width {
            let byte_index = start + row * row_bytes + column / 8;
            if byte_index < font.len() && font[byte_index] & (0x80 >> (column & 7)) != 0 {
                pixel(
                    console,
                    console.cursor_x + column,
                    console.cursor_y + row,
                    0xdce8e8,
                );
            }
        }
    }
    console.cursor_x += console.font_width;
}

fn glyph_index(console: &Console, font: &[u8], character: char) -> Option<usize> {
    if (character as usize) < console.glyph_count {
        return Some(character as usize);
    }
    if !console.has_unicode_table || console.unicode_offset >= font.len() {
        return None;
    }
    if font.starts_with(&[0x36, 0x04]) {
        let mut offset = console.unicode_offset;
        for glyph in 0..console.glyph_count {
            while offset + 2 <= font.len() {
                let codepoint = u16::from_le_bytes([font[offset], font[offset + 1]]);
                offset += 2;
                if codepoint == 0xffff {
                    break;
                }
                if codepoint == 0xfffe {
                    while offset + 2 <= font.len() {
                        let sequence = u16::from_le_bytes([font[offset], font[offset + 1]]);
                        offset += 2;
                        if sequence == 0xffff {
                            break;
                        }
                    }
                    break;
                }
                if codepoint as u32 == character as u32 {
                    return Some(glyph);
                }
            }
        }
        return None;
    }

    let mut offset = console.unicode_offset;
    for glyph in 0..console.glyph_count {
        while offset < font.len() && font[offset] != 0xff {
            if font[offset] == 0xfe {
                offset += 1;
                while offset < font.len() && font[offset] != 0xff {
                    offset += 1;
                }
                break;
            }
            let start = offset;
            while offset < font.len() && font[offset] != 0xfe && font[offset] != 0xff {
                offset += 1;
            }
            if core::str::from_utf8(&font[start..offset])
                .ok()?
                .chars()
                .any(|value| value == character)
            {
                return Some(glyph);
            }
        }
        if offset < font.len() && font[offset] == 0xff {
            offset += 1;
        } else {
            break;
        }
    }
    None
}

fn draw_builtin(console: &Console, character: char) {
    let pattern: [u8; 5] = match character.to_ascii_uppercase() {
        'A' => [0x7e, 0x11, 0x11, 0x11, 0x7e],
        'B' => [0x7f, 0x49, 0x49, 0x49, 0x36],
        'C' => [0x3e, 0x41, 0x41, 0x41, 0x22],
        'D' => [0x7f, 0x41, 0x41, 0x22, 0x1c],
        'E' => [0x7f, 0x49, 0x49, 0x49, 0x41],
        'F' => [0x7f, 0x09, 0x09, 0x09, 0x01],
        'G' => [0x3e, 0x41, 0x49, 0x49, 0x7a],
        'H' => [0x7f, 0x08, 0x08, 0x08, 0x7f],
        'I' => [0x00, 0x41, 0x7f, 0x41, 0x00],
        'J' => [0x20, 0x40, 0x41, 0x3f, 0x01],
        'K' => [0x7f, 0x08, 0x14, 0x22, 0x41],
        'L' => [0x7f, 0x40, 0x40, 0x40, 0x40],
        'M' => [0x7f, 0x02, 0x0c, 0x02, 0x7f],
        'N' => [0x7f, 0x04, 0x08, 0x10, 0x7f],
        'O' => [0x3e, 0x41, 0x41, 0x41, 0x3e],
        'P' => [0x7f, 0x09, 0x09, 0x09, 0x06],
        'Q' => [0x3e, 0x41, 0x51, 0x21, 0x5e],
        'R' => [0x7f, 0x09, 0x19, 0x29, 0x46],
        'S' => [0x46, 0x49, 0x49, 0x49, 0x31],
        'T' => [0x01, 0x01, 0x7f, 0x01, 0x01],
        'U' => [0x3f, 0x40, 0x40, 0x40, 0x3f],
        'V' => [0x1f, 0x20, 0x40, 0x20, 0x1f],
        'W' => [0x3f, 0x40, 0x38, 0x40, 0x3f],
        'X' => [0x63, 0x14, 0x08, 0x14, 0x63],
        'Y' => [0x07, 0x08, 0x70, 0x08, 0x07],
        'Z' => [0x61, 0x51, 0x49, 0x45, 0x43],
        '0' => [0x3e, 0x51, 0x49, 0x45, 0x3e],
        '1' => [0x00, 0x42, 0x7f, 0x40, 0x00],
        '2' => [0x42, 0x61, 0x51, 0x49, 0x46],
        '3' => [0x21, 0x41, 0x45, 0x4b, 0x31],
        '4' => [0x18, 0x14, 0x12, 0x7f, 0x10],
        '5' => [0x27, 0x45, 0x45, 0x45, 0x39],
        '6' => [0x3c, 0x4a, 0x49, 0x49, 0x30],
        '7' => [0x01, 0x71, 0x09, 0x05, 0x03],
        '8' => [0x36, 0x49, 0x49, 0x49, 0x36],
        '9' => [0x06, 0x49, 0x49, 0x29, 0x1e],
        ':' => [0x00, 0x36, 0x36, 0x00, 0x00],
        '.' => [0x00, 0x60, 0x60, 0x00, 0x00],
        ',' => [0x00, 0x40, 0x30, 0x00, 0x00],
        '-' => [0x08, 0x08, 0x08, 0x08, 0x08],
        '_' => [0x40, 0x40, 0x40, 0x40, 0x40],
        '/' => [0x20, 0x10, 0x08, 0x04, 0x02],
        '>' => [0x00, 0x41, 0x22, 0x14, 0x08],
        '<' => [0x08, 0x14, 0x22, 0x41, 0x00],
        '?' => [0x02, 0x01, 0x51, 0x09, 0x06],
        '!' => [0x00, 0x00, 0x5f, 0x00, 0x00],
        '\'' => [0x00, 0x03, 0x01, 0x00, 0x00],
        '"' => [0x03, 0x00, 0x03, 0x00, 0x00],
        '#' => [0x14, 0x7f, 0x14, 0x7f, 0x14],
        '=' => [0x14, 0x14, 0x14, 0x14, 0x14],
        ' ' => [0; 5],
        _ => [0x7f, 0x41, 0x5d, 0x41, 0x7f],
    };
    for (column, bits) in pattern.iter().enumerate() {
        for row in 0..7 {
            if bits & (1 << row) != 0 {
                pixel(
                    console,
                    console.cursor_x + column,
                    console.cursor_y + row,
                    0xdce8e8,
                );
            }
        }
    }
}

fn pixel(console: &Console, x: usize, y: usize, color: u32) {
    if x >= console.width || y >= console.height {
        return;
    }
    let red = ((color >> 16) & 0xff) << console.red_shift;
    let green = ((color >> 8) & 0xff) << console.green_shift;
    let blue = (color & 0xff) << console.blue_shift;
    let value = red | green | blue;
    unsafe {
        ptr::write_volatile(
            console.address.add(y * console.pitch + x * 4).cast::<u32>(),
            value,
        );
    }
}

fn read_u32(bytes: &[u8], offset: usize) -> Option<u32> {
    Some(u32::from_le_bytes(
        bytes.get(offset..offset + 4)?.try_into().ok()?,
    ))
}

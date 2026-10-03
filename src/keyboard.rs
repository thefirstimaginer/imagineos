use core::arch::asm;

pub struct Keyboard {
    shift: bool,
    caps_lock: bool,
    extended: bool,
    alt_gr: bool,
}

impl Keyboard {
    pub const fn new() -> Self {
        Self {
            shift: false,
            caps_lock: false,
            extended: false,
            alt_gr: false,
        }
    }

    pub fn poll_char(&mut self) -> Option<char> {
        unsafe {
            if in_port(0x64) & 1 == 0 {
                return None;
            }
            let code = in_port(0x60);
            if code == 0xe0 {
                self.extended = true;
                return None;
            }

            let released = code & 0x80 != 0;
            let key = code & 0x7f;
            match key {
                0x2a | 0x36 => self.shift = !released,
                0x3a if !released => self.caps_lock = !self.caps_lock,
                0x38 if self.extended => self.alt_gr = !released,
                _ => {}
            }
            let extended = core::mem::replace(&mut self.extended, false);
            if released || extended {
                return None;
            }
            translate(key, self.shift, self.caps_lock, self.alt_gr)
        }
    }
}

fn translate(code: u8, shift: bool, caps: bool, alt_gr: bool) -> Option<char> {
    if alt_gr {
        return match code {
            0x10 => Some('ä'),
            0x11 => Some('å'),
            0x12 => Some('€'),
            0x19 => Some('þ'),
            0x1e => Some('æ'),
            0x2c => Some('ø'),
            _ => None,
        };
    }
    let shifted = shift ^ caps;
    let character = match code {
        0x02..=0x0b => {
            const NORMAL: &[u8] = b"1234567890";
            const UPPER: &[u8] = b"!@#$%^&*()";
            if shifted {
                UPPER[(code - 0x02) as usize]
            } else {
                NORMAL[(code - 0x02) as usize]
            }
        }
        0x10..=0x19 => {
            (if shifted {
                b"QWERTYUIOP"
            } else {
                b"qwertyuiop"
            })[(code - 0x10) as usize]
        }
        0x1e..=0x26 => (if shifted { b"ASDFGHJKL" } else { b"asdfghjkl" })[(code - 0x1e) as usize],
        0x2c..=0x32 => (if shifted { b"ZXCVBNM" } else { b"zxcvbnm" })[(code - 0x2c) as usize],
        0x1c => return Some('\n'),
        0x0e => return Some('\u{8}'),
        0x39 => return Some(' '),
        0x0c => {
            if shifted {
                b'_'
            } else {
                b'-'
            }
        }
        0x0d => {
            if shifted {
                b'+'
            } else {
                b'='
            }
        }
        0x1a => {
            if shifted {
                b'{'
            } else {
                b'['
            }
        }
        0x1b => {
            if shifted {
                b'}'
            } else {
                b']'
            }
        }
        0x27 => {
            if shifted {
                b':'
            } else {
                b';'
            }
        }
        0x28 => {
            if shifted {
                b'"'
            } else {
                b'\''
            }
        }
        0x29 => {
            if shifted {
                b'~'
            } else {
                b'`'
            }
        }
        0x2b => {
            if shifted {
                b'|'
            } else {
                b'\\'
            }
        }
        0x33 => {
            if shifted {
                b'<'
            } else {
                b','
            }
        }
        0x34 => {
            if shifted {
                b'>'
            } else {
                b'.'
            }
        }
        0x35 => {
            if shifted {
                b'?'
            } else {
                b'/'
            }
        }
        _ => return None,
    };
    Some(character as char)
}

unsafe fn in_port(port: u16) -> u8 {
    let value: u8;
    asm!("in al, dx", in("dx") port, out("al") value, options(nomem, nostack, preserves_flags));
    value
}

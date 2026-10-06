#![no_std]
#![no_main]

const TARGET_NAME: &[u8] = b"/dev/hda";
const FIRST_CONFIRMATION: &[u8] = b"APAGAR /dev/hda";
const SECOND_CONFIRMATION: &[u8] = b"INSTALAR";

fn run() -> imagineos::Result<()> {
    let disks = imagineos::fs::disk_count()?;
    if disks == 0 {
        imagineos::console::write_all(b"distroinstall: nenhum disco encontrado em /dev\n")?;
        return Err(imagineos::Error::from_errno(19));
    }

    imagineos::console::write_all(b"Discos disponiveis:\n")?;
    for index in 0..disks {
        let sectors = imagineos::fs::disk_sector_count(index)?;
        imagineos::console::write_all(b"  /dev/hda - ")?;
        write_number(sectors / 2048)?;
        imagineos::console::write_all(b" MiB\n")?;
    }

    let mut selected = [0u8; 64];
    imagineos::console::write_all(b"Disco de destino [/dev/hda]: ")?;
    let length = read_line(&mut selected)?;
    if length != 0 && &selected[..length] != TARGET_NAME {
        imagineos::console::write_all(b"distroinstall: destino nao suportado\n")?;
        return Err(imagineos::Error::from_errno(19));
    }

    imagineos::console::write_all(
        b"\nATENCAO: todos os dados e particoes de /dev/hda serao apagados.\n\
          A instalacao cria uma ESP UEFI e reserva o restante para o DFS.\n\
          O ImagineOS continuara usando o RAMFS volatil nesta versao.\n\
          Digite `APAGAR /dev/hda` para a primeira confirmacao: ",
    )?;
    let mut confirmation = [0u8; 64];
    let length = read_line(&mut confirmation)?;
    if &confirmation[..length] != FIRST_CONFIRMATION {
        imagineos::console::write_all(b"Instalacao cancelada.\n")?;
        return Ok(());
    }

    imagineos::console::write_all(
        b"Ultima confirmacao: digite `INSTALAR` para particionar e formatar /dev/hda: ",
    )?;
    let length = read_line(&mut confirmation)?;
    if &confirmation[..length] != SECOND_CONFIRMATION {
        imagineos::console::write_all(b"Instalacao cancelada; o disco nao foi alterado.\n")?;
        return Ok(());
    }

    imagineos::console::write_all(
        b"Instalando. Nao desligue o computador; uma falha de I/O pode deixar o disco inutilizavel.\n",
    )?;
    imagineos::fs::install_to_disk(0)?;
    imagineos::console::write_all(
        b"ImagineOS instalado em /dev/hda. Reinicie para testar o boot pelo disco.\n",
    )
}

fn read_line(output: &mut [u8]) -> imagineos::Result<usize> {
    let mut length = 0usize;
    loop {
        let character = imagineos::console::read_char()?;
        match character {
            '\r' | '\n' => {
                imagineos::console::write_all(b"\n")?;
                return Ok(length);
            }
            '\u{8}' | '\u{7f}' => {
                if length != 0 {
                    length -= 1;
                    imagineos::console::write_all(b"\x08 \x08")?;
                }
            }
            character if character.is_ascii() && !character.is_ascii_control() => {
                if length < output.len() {
                    output[length] = character as u8;
                    length += 1;
                    imagineos::console::write_all(&[character as u8])?;
                } else {
                    imagineos::console::write_all(b"\x07")?;
                }
            }
            _ => {}
        }
    }
}

fn write_number(mut value: u64) -> imagineos::Result<()> {
    let mut digits = [0u8; 20];
    let mut cursor = digits.len();
    if value == 0 {
        return imagineos::console::write_all(b"0");
    }
    while value != 0 {
        cursor -= 1;
        digits[cursor] = b'0' + (value % 10) as u8;
        value /= 10;
    }
    imagineos::console::write_all(&digits[cursor..])
}

#[no_mangle]
extern "C" fn _start(
    _argc: usize,
    _argv: *const *const u8,
    _envc: usize,
    _envp: *const *const u8,
) -> ! {
    if let Err(error) = run() {
        let _ = imagineos::console::write_error(b"distroinstall: falha ao instalar no disco\n");
        imagineos::process::exit(error.code());
    }
    imagineos::process::exit(0)
}

#[panic_handler]
fn panic(_info: &core::panic::PanicInfo<'_>) -> ! {
    imagineos::process::exit(127)
}

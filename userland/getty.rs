#![no_std]
#![no_main]

use core::panic::PanicInfo;

#[no_mangle]
extern "C" fn _start(
    _argc: usize,
    _argv: *const *const u8,
    _envc: usize,
    _envp: *const *const u8,
) -> ! {
    if imagineos::console::write_all(
        b"Astrid getty: console ready\n\nWelcome to Imagine Operating System!\n\n",
    )
    .is_err()
    {
        imagineos::process::exit(1);
    }
    let shell_pid = match imagineos::process::exec("/bin/shell", &["/bin/shell"], &[]) {
        Ok(pid) => pid,
        Err(_) => imagineos::process::exit(1),
    };
    loop {
        let mut processes = [imagineos::abi::ProcessInfo::default(); imagineos::abi::MAX_PROCESSES];
        match imagineos::process::list(&mut processes) {
            Ok(count)
                if processes
                    .iter()
                    .take(count)
                    .any(|process| process.pid == shell_pid as u64) =>
            {
                if imagineos::process::yield_now().is_err() {
                    imagineos::process::exit(1);
                }
            }
            Ok(_) => break,
            Err(_) => imagineos::process::exit(1),
        }
    }
    imagineos::process::exit(0)
}

#[panic_handler]
fn panic(_info: &PanicInfo<'_>) -> ! {
    imagineos::process::exit(127)
}

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
    if imagineos::console::write_all(b"Astrid init: PID 1 supervising the system\n").is_err() {
        imagineos::process::exit(1);
    }
    let mut getty_pid = 0;
    loop {
        if getty_pid == 0 || !process_is_active(getty_pid) {
            getty_pid = match imagineos::process::exec("/sbin/getty", &["/sbin/getty"], &[]) {
                Ok(pid) => pid,
                Err(_) => {
                    let _ = imagineos::console::write_all(b"init: unable to start getty\n");
                    let _ = imagineos::process::yield_now();
                    continue;
                }
            };
        }
        match imagineos::shutdown::take_request() {
            Ok(true) => {
                if imagineos::console::write_all(
                    b"init: shutdown requested; syncing storage and powering off\n",
                )
                .is_err()
                {
                    imagineos::process::exit(1);
                }
                if imagineos::shutdown::power_off().is_err() {
                    let _ = imagineos::console::write_all(
                        b"init: shutdown failed; system remains running\n",
                    );
                }
            }
            Ok(false) => {}
            Err(_) => {
                let _ = imagineos::console::write_all(b"init: shutdown IPC failed\n");
            }
        }
        if imagineos::process::yield_now().is_err() {
            imagineos::process::exit(1);
        }
    }
}

fn process_is_active(pid: usize) -> bool {
    let mut processes = [imagineos::abi::ProcessInfo::default(); imagineos::abi::MAX_PROCESSES];
    imagineos::process::list(&mut processes)
        .ok()
        .is_some_and(|count| {
            processes
                .iter()
                .take(count)
                .any(|process| process.pid == pid as u64)
        })
}

#[panic_handler]
fn panic(_info: &PanicInfo<'_>) -> ! {
    imagineos::process::exit(127)
}

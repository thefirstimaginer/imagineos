// userland/utilities/hello.rs
#![no_std]
#![no_main]

use imagineos_rt::{main, println};

fn user_main() -> Result<(), i32> {
    println!("Hello from ImagineOS!");
    println!("Este é um programa limpo rodando na ABI v1!");

    let x = 10;
    let y = 20;
    println!("x = {}, y = {}", x, y);
    println!("x + y = {}", x + y);
    
    Ok(())
}

// Expande o _start, valida a ABI_VERSION e chama user_main()
main!(user_main);
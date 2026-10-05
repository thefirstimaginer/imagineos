/*
    A simple implementation of the `ls` command.
    (C) 2026 The Imagine Project. All rights reserved.

    ----------
    DNU Public License v1.0 (DNU-PLv1)

    This program is free software: you can redistribute it and/or modify
    it under the terms of the DNU Public License as published by
    The Imagine Project, either version 1 of the License, or (at your option) 
    any later version.
*/
#![no_std]
#![no_main]

#[allow(dead_code)]
mod common;

#[no_mangle]
extern "C" fn _start(
    argc: usize,
    argv: *const *const u8,
    envc: usize,
    envp: *const *const u8,
) -> ! {
    // Determine the requested path to list. If no argument is provided,
    //  use the current working directory from the environment variables.
    let requested = if argc > 1 {
        unsafe { common::argument(argv, 1).unwrap_or(b"/") }
    } else {
        unsafe { common::environment_value(envp, envc, b"PWD").unwrap_or(b"/") }
    };
    // Resolve the requested path to an absolute path.
    let mut path = [0u8; 256];
    let Some(path_length) = common::resolve_path(envp, envc, requested, &mut path) else {
        common::write(b"ls: path too long\n");
        common::exit(2);
    };
    // List the directory entries for the resolved path.
    let mut entries = [0u8; 2048];
    let result = common::list_directory(&path[..path_length], &mut entries);
    if result < 0 {
        common::write(b"ls: directory not found\n");
        common::exit(2);
    }
    common::write(&entries[..result as usize]);
    common::exit(0)
}

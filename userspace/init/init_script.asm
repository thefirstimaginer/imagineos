global init_script_start
global init_script_end

section .rodata
init_script_start:
    incbin "userspace/init/init.d/01-shell"
init_script_end:

global init_script_start
global init_script_end

section .rodata
init_script_start:
    incbin "userspace/init/initfile/initfile.ini"
init_script_end:

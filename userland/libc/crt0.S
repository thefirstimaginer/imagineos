.text
.global _start
.type _start, @function
_start:
    xorq %rbp, %rbp
    andq $-16, %rsp
    call c_runtime_init
    ud2
.size _start, .-_start

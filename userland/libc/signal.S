.text
.global dc_signal_restorer
.type dc_signal_restorer, @function
dc_signal_restorer:
    movq $31, %rax
    int $0x80
    ud2
.size dc_signal_restorer, .-dc_signal_restorer

.section .note.GNU-stack,"",@progbits

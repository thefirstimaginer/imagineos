ENTRY(start)

SECTIONS {
    /* Ocupa o espaço inicial para garantir que o header fique perto do topo */
    . = 1M;

    .boot : ALIGN(4K) {
        /* O KEEP garante que o compilador não jogue fora o header por achar "inútil" */
        KEEP(*(.multiboot_header))
    }

    .text : ALIGN(4K) {
        *(.text)
    }
    
    /* Mova o resto para depois, para não empurrar o header para baixo */
    .rodata : ALIGN(4K) { *(.rodata) }
    .data : ALIGN(4K) { *(.data) }
}
ENTRY(_start)

BOOTSTRAP_VMA = 0xffffffffc0000000;
BOOTSTRAP_LMA = 0x02000000;

SECTIONS
{
    . = BOOTSTRAP_VMA + BOOTSTRAP_LMA;

    .requests : AT(ADDR(.requests) - BOOTSTRAP_VMA) ALIGN(8) {
        KEEP(*(.requests_start_marker))
        KEEP(*(.requests))
        KEEP(*(.requests_end_marker))
    }

    .text : AT(ADDR(.text) - BOOTSTRAP_VMA) ALIGN(4096) { *(.text .text.*) }
    .rodata : AT(ADDR(.rodata) - BOOTSTRAP_VMA) ALIGN(4096) { *(.rodata .rodata.*) }
    .data : AT(ADDR(.data) - BOOTSTRAP_VMA) ALIGN(4096) { *(.got .got.*) *(.data .data.*) }
    .bss : AT(ADDR(.bss) - BOOTSTRAP_VMA) ALIGN(4096) { *(COMMON) *(.bss .bss.*) }

    /DISCARD/ : { *(.eh_frame .eh_frame_hdr) }
}

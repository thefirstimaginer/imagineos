ENTRY(_start)

SECTIONS
{
    . = 0x100000;

    .requests : ALIGN(8) {
        KEEP(*(.requests_start_marker))
        KEEP(*(.requests))
        KEEP(*(.requests_end_marker))
    }

    .text : ALIGN(4096) { *(.text .text.*) }
    .rodata : ALIGN(4096) { *(.rodata .rodata.*) }
    .data : ALIGN(4096) { *(.data .data.*) }
    .bss : ALIGN(4096) { *(COMMON) *(.bss .bss.*) }

    /DISCARD/ : { *(.eh_frame .eh_frame_hdr) }
}
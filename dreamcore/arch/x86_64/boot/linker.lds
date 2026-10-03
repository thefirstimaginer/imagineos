OUTPUT_FORMAT(elf64-x86-64)
OUTPUT_ARCH(i386:x86-64)

ENTRY(_start)

PHDRS
{
    limine_requests PT_LOAD;
    text            PT_LOAD;
    rodata          PT_LOAD;
    data            PT_LOAD;
}

SECTIONS
{
    /* Endereço virtual alto recomendado */
    . = 0xffffffff80000000;

    .limine_requests : ALIGN(16)
    {
        KEEP(*(.limine_requests_start))
        KEEP(*(.limine_requests))
        KEEP(*(.limine_requests_end))
    } :limine_requests

    . = ALIGN(4096);

    .text :
    {
        __text_start = .;

        *(.text)
        *(.text.*)

        __text_end = .;
    } :text

    . = ALIGN(4096);

    .rodata :
    {
        __rodata_start = .;

        *(.rodata)
        *(.rodata.*)

        __rodata_end = .;
    } :rodata

    .note.gnu.build-id :
    {
        *(.note.gnu.build-id)
    } :rodata

    . = ALIGN(4096);

    .data :
    {
        __data_start = .;

        *(.data)
        *(.data.*)

        __data_end = .;
    } :data

    .bss :
    {
        __bss_start = .;

        *(.bss)
        *(.bss.*)
        *(COMMON)

        __bss_end = .;
    } :data

    __kernel_start = ADDR(.text);
    __kernel_end = .;

    /DISCARD/ :
    {
        *(.eh_frame*)
        *(.note)
        *(.note.*)
        *(.comment)
    }
}
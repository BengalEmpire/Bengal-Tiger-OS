ENTRY(start)

SECTIONS
{
    . = 0x00100000; /* Link the kernel at 1MB (0x00100000) */

    .text ALIGN(4096) : AT(ADDR(.text))
    {
        KEEP(*(.multiboot)) /* Multiboot header MUST reside within first 8KB */
        *(.text .text.*)
    }

    .rodata ALIGN(4096) : AT(ADDR(.rodata))
    {
        *(.rodata .rodata.*)
    }

    .data ALIGN(4096) : AT(ADDR(.data))
    {
        *(.data .data.*)
    }

    .bss ALIGN(4096) : AT(ADDR(.bss))
    {
        *(.bss .bss.*)
        *(COMMON)
        . = ALIGN(4096);
        PROVIDE(bss_end = .);
    }

    /DISCARD/ :
    {
        *(.eh_frame)
        *(.comment)
        *(.note.gnu.property)
    }
}

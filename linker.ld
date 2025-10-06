ENTRY(start)
OUTPUT_FORMAT(elf32-i386)
SECTIONS {
    . = 1M; /* load address 1MiB */
    .multiboot : { *(.multiboot) }
    .text : { *(.text) }
    .rodata : { *(.rodata) }
    .data : { *(.data) }
    .bss : {
        bss_start = .;
        *(.bss) *(COMMON)
        bss_end = .;
    }
    /DISCARD/ : { *(.comment) (.note) }
}
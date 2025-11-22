ENTRY(start)

SECTIONS
{
	. = 0x00100000; /* Link the kernel at 1MB (0x00100000) */

	.text : AT(ADDR(.text))
	{
		KEEP(*(.multiboot)) /* Place the multiboot header FIRST */
		*(.text)
		*(.rodata)
	}

	.data : {
		*(.data)
	}

	.bss : {
		*(.bss)
		*(COMMON)
		/* *** CRITICAL FIX: DEFINE THE bss_end SYMBOL *** */
		PROVIDE(bss_end = .); 
	}

	/DISCARD/ : { *(.eh_frame) *(.comment) }
}
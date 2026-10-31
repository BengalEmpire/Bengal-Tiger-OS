/* Bengal Tiger OS - Multiboot1 Entry & Bootstrap Stack
 *
 * Compliant with Multiboot Specification v0.6.96 for ELF32 images:
 *   Bit 0 (0x01): Page-align boot modules
 *   Bit 1 (0x02): Request BIOS/E820 memory map
 *   Bit 2 (0x04): Request linear framebuffer / video mode info
 *   Bit 16 = 0  : Use ELF32 program headers (do NOT set a.out kludge)
 */

    .set MULTIBOOT_MAGIC,    0x1BADB002
    .set MULTIBOOT_FLAGS,    0x00000007
    .set MULTIBOOT_CHECKSUM, -(MULTIBOOT_MAGIC + MULTIBOOT_FLAGS)

    .section .multiboot, "a"
    .align 4
    .long MULTIBOOT_MAGIC
    .long MULTIBOOT_FLAGS
    .long MULTIBOOT_CHECKSUM
    /* Address fields (offsets 12..31, unused when bit 16 is clear) */
    .long 0
    .long 0
    .long 0
    .long 0
    .long 0
    /* Video mode preference (offsets 32..47, used when bit 2 is set) */
    .long 0     /* mode_type: 0 = linear graphics (overridden by GRUB gfxpayload) */
    .long 1024  /* width */
    .long 768   /* height */
    .long 32    /* depth */

    .section .bss
    .align 16
    .global stack_bottom
    .global stack_top
stack_bottom:
    .skip 32768 /* 32 KB 16-byte-aligned kernel bootstrap stack */
stack_top:

    .section .text
    .global start
start:
    cli
    cld                     /* Ensure forward string operations per System V i386 ABI */
    mov $stack_top, %esp
    xor %ebp, %ebp
    sub $8, %esp            /* Maintain 16-byte stack alignment before call */
    push %eax               /* arg2: Multiboot bootloader magic (0x2BADB002) */
    push %ebx               /* arg1: Multiboot info structure pointer */
    call kmain
    cli
1:
    hlt
    jmp 1b

/* Mark stack as non-executable to suppress linker warning */
.section .note.GNU-stack,"",@progbits

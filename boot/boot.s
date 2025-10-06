/* boot/boot.s - multiboot header and entry */
    .section .multiboot
    .align 4
    .long 0x1BADB002        /* magic */
    .long 0x00010003        /* flags: request memory info + a.out kludge ok */
    .long -(0x1BADB002 + 0x00010003)

    .section .text
    .global start
start:
    push %ebx  /* Push multiboot info pointer */
    /* call C kernel entry */
    call kmain

    /* halt if kmain returns */
1:
    cli
    hlt
    jmp 1b
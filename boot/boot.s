    .section .multiboot
    .align 4
    .long 0x1BADB002
    .long 0x00010003
    .long -(0x1BADB002 + 0x00010003)

    .section .text
    .global start
start:
    push %ebx  /* Push multiboot info pointer */
    call kmain
1:
    cli
    hlt
    jmp 1b
    .section .multiboot
    .align 4
    .long 0x1BADB002
    .long 0x00010003
    .long -(0x1BADB002 + 0x00010003)

    .section .text
    .global start
start:
    mov $0x9BFFF, %esp
    push %ebx
    call kmain
    cli
    hlt
1:
    jmp 1b

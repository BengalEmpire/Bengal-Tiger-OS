.section .text

.macro ISR_NOERRCODE num
.global isr\num
isr\num:
    cli
    push $0
    push $\num
    jmp isr_common
.endm

.macro ISR_ERRCODE num
.global isr\num
isr\num:
    cli
    push $\num
    jmp isr_common
.endm

.macro IRQ num irqnum
.global irq\num
irq\num:
    cli
    push $0
    push $\irqnum
    jmp irq_common
.endm

ISR_NOERRCODE 0
ISR_ERRCODE 8  // Example, add all 0-31 as per OSDev

IRQ 0, 32
IRQ 1, 33
// Add IRQ 2-15

.extern isr_handler
.extern irq_handler

isr_common:
    pusha
    push %ds
    push %es
    push %fs
    push %gs
    mov $0x10, %ax
    mov %ax, %ds
    mov %ax, %es
    mov %ax, %fs
    mov %ax, %gs
    push %esp
    call isr_handler
    pop %esp
    pop %gs
    pop %fs
    pop %es
    pop %ds
    popa
    add $8, %esp
    iret

irq_common:
    pusha
    push %ds
    push %es
    push %fs
    push %gs
    mov $0x10, %ax
    mov %ax, %ds
    mov %ax, %es
    mov %ax, %fs
    mov %ax, %gs
    push %esp
    call irq_handler
    pop %esp
    pop %gs
    pop %fs
    pop %es
    pop %ds
    popa
    add $8, %esp
    iret
.section .text
.align 4

# --- EXTERNALS ---
.extern isr_handler
.extern irq_handler

# --- GLOBAL FUNCTIONS ---
.global idt_load

# --- MACRO DEFINITIONS (MUST BE FIRST) ---

# Macro for Interrupts with NO error code (pushes dummy 0)
.macro ISR_NOERRCODE num
.global isr\num
isr\num:
    cli
    push $0         # Push dummy error code
    push $\num      # Push interrupt number
    jmp isr_common
.endm

# Macro for Interrupts WITH error code (CPU pushes error code automatically)
.macro ISR_ERRCODE num
.global isr\num
isr\num:
    cli
    push $\num      # Push interrupt number
    jmp isr_common
.endm

# Macro for Hardware Interrupts (IRQs)
.macro IRQ num, idt_index
.global irq\num
irq\num:
    cli
    push $0         # Push dummy error code
    push $\idt_index # Push IDT index (32-47)
    jmp irq_common
.endm

# --- IDT LOAD FUNCTION ---
idt_load:
    mov 4(%esp), %eax
    lidt (%eax)
    ret

# --- INTERRUPT SERVICE ROUTINES (0-31) ---
ISR_NOERRCODE 0
ISR_NOERRCODE 1
ISR_NOERRCODE 2
ISR_NOERRCODE 3
ISR_NOERRCODE 4
ISR_NOERRCODE 5
ISR_NOERRCODE 6
ISR_NOERRCODE 7
ISR_ERRCODE   8
ISR_NOERRCODE 9
ISR_ERRCODE   10
ISR_ERRCODE   11
ISR_ERRCODE   12
ISR_ERRCODE   13
ISR_ERRCODE   14
ISR_NOERRCODE 15
ISR_NOERRCODE 16
ISR_ERRCODE   17
ISR_NOERRCODE 18
ISR_NOERRCODE 19
ISR_NOERRCODE 20
ISR_NOERRCODE 21
ISR_NOERRCODE 22
ISR_NOERRCODE 23
ISR_NOERRCODE 24
ISR_NOERRCODE 25
ISR_NOERRCODE 26
ISR_NOERRCODE 27
ISR_NOERRCODE 28
ISR_NOERRCODE 29
ISR_NOERRCODE 30
ISR_NOERRCODE 31

# --- HARDWARE INTERRUPTS (IRQs) ---
# Map IRQ 0-15 to IDT 32-47
IRQ 0, 32
IRQ 1, 33
IRQ 2, 34
IRQ 3, 35
IRQ 4, 36
IRQ 5, 37
IRQ 6, 38
IRQ 7, 39
IRQ 8, 40
IRQ 9, 41
IRQ 10, 42
IRQ 11, 43
IRQ 12, 44
IRQ 13, 45
IRQ 14, 46
IRQ 15, 47

# --- SYSTEM CALL (User Mode) ---
ISR_NOERRCODE 128

# --- COMMON HANDLERS ---

# Common ISR Stub
isr_common:
    pusha               # Pushes edi, esi, ebp, esp, ebx, edx, ecx, eax
    push %ds
    push %es
    push %fs
    push %gs
    
    mov $0x10, %ax      # Load Kernel Data Segment descriptor
    mov %ax, %ds
    mov %ax, %es
    mov %ax, %fs
    mov %ax, %gs
    
    mov %esp, %eax      # Push pointer to stack (struct regs)
    push %eax
    
    call isr_handler    # Call C function
    
    pop %eax            # Pop stack pointer
    pop %gs
    pop %fs
    pop %es
    pop %ds
    popa
    add $8, %esp        # Cleans up the pushed error code and ISR number
    iret

# Common IRQ Stub
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
    
    mov %esp, %eax
    push %eax
    
    call irq_handler
    
    pop %eax
    pop %gs
    pop %fs
    pop %es
    pop %ds
    popa
    add $8, %esp
    iret
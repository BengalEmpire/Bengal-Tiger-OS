/**
 * Bengal Tiger OS - Interrupt Service Routines (C Handlers)
 * 
 * These are the C-language handler functions called by the
 * assembly stubs in isr.s.
 * 
 * @file isr.c
 * @author Bengal Tiger OS Team
 * @version 0.3.0
 */

#include "common.h"
#include "keyboard.h"
#include "timer.h"
#include "panic.h"

/**
 * CPU Exception Handler
 * 
 * Called for interrupts 0-31 (CPU exceptions).
 * Most exceptions are fatal and will trigger a kernel panic.
 * 
 * @param r Pointer to saved CPU registers
 */
void isr_handler(struct regs *r) {
    /* Handle the exception based on interrupt number */
    exception_handler(r);
}

/**
 * Hardware IRQ Handler
 * 
 * Called for interrupts 32-47 (remapped IRQs 0-15).
 * Dispatches to appropriate device driver handlers.
 * 
 * @param r Pointer to saved CPU registers
 */
void irq_handler(struct regs *r) {
    /* Send EOI (End of Interrupt) to PICs */
    
    /* If this was from Slave PIC (IRQ 8-15, INT 40-47) */
    if (r->int_no >= 40) {
        outb(0xA0, 0x20);  /* Slave PIC EOI */
    }
    
    /* Always send to Master PIC */
    outb(0x20, 0x20);  /* Master PIC EOI */
    
    /* Dispatch to appropriate handler */
    switch (r->int_no) {
        case 32:  /* IRQ0 - Timer */
            timer_handler(r);
            break;
            
        case 33:  /* IRQ1 - Keyboard */
            keyboard_handler();
            break;
            
        case 34:  /* IRQ2 - Cascade (internal) */
            break;
            
        case 35:  /* IRQ3 - COM2 */
            break;
            
        case 36:  /* IRQ4 - COM1 */
            break;
            
        case 37:  /* IRQ5 - LPT2 */
            break;
            
        case 38:  /* IRQ6 - Floppy Disk */
            break;
            
        case 39:  /* IRQ7 - LPT1 / Spurious */
            /* IRQ7 can be spurious - verify by checking ISR */
            break;
            
        case 40:  /* IRQ8 - CMOS RTC */
            break;
            
        case 41:  /* IRQ9 - Free / ACPI */
            break;
            
        case 42:  /* IRQ10 - Free */
            break;
            
        case 43:  /* IRQ11 - Free */
            break;
            
        case 44:  /* IRQ12 - PS/2 Mouse */
            break;
            
        case 45:  /* IRQ13 - FPU */
            break;
            
        case 46:  /* IRQ14 - Primary ATA */
            break;
            
        case 47:  /* IRQ15 - Secondary ATA */
            break;
            
        default:
            /* Unknown IRQ - ignore */
            break;
    }
}

/**
 * System Call Handler (INT 0x80)
 * 
 * Handles software interrupt 0x80 for system calls.
 * Currently not implemented - reserved for future use.
 * 
 * @param r Pointer to saved CPU registers
 */
void syscall_handler(struct regs *r) {
    /* System call number in EAX */
    uint32_t syscall_num = r->eax;
    
    /* TODO: Implement system call dispatch */
    (void)syscall_num;
    
    /* Return value goes in EAX */
    r->eax = 0;
}
/**
 * Bengal Tiger OS - Interrupt Service Routines (C Handlers)
 * 
 * These are the C-language handler functions called by the
 * assembly stubs in isr.s.
 * 
 * @file isr.c
 * @author Bengal Tiger OS Team
 * @version 0.6.0
 */

#include "common.h"
#include "keyboard.h"
#include "timer.h"
#include "panic.h"
#include "mouse.h"
#include "scheduler.h"

/**
 * CPU Exception Handler
 */
void isr_handler(struct regs *r) {
    exception_handler(r);
}

/**
 * Hardware IRQ Handler
 * Returns stack pointer for task switching.
 */
struct regs* irq_handler(struct regs *r) {
    /* Send EOI (End of Interrupt) to PICs */
    if (r->int_no >= 40) {
        outb(0xA0, 0x20);  /* Slave PIC EOI */
    }
    outb(0x20, 0x20);      /* Master PIC EOI */
    
    struct regs *ret_r = r;

    switch (r->int_no) {
        case 32:  /* IRQ0 - Timer */
            timer_handler(r);
            ret_r = scheduler_tick(r);
            break;
            
        case 33:  /* IRQ1 - Keyboard */
            keyboard_handler();
            break;
            
        case 44:  /* IRQ12 - PS/2 Mouse */
            mouse_handler();
            break;
            
        default:
            break;
    }

    return ret_r;
}

void syscall_handler(struct regs *r) {
    uint32_t syscall_num = r->eax;
    (void)syscall_num;
    r->eax = 0;
}

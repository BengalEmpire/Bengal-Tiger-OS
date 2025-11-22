#include "common.h"
#include "keyboard.h"
#include "scheduler.h"

void isr_handler(struct regs *r) {
    // Handle exceptions (stub)
}

void irq_handler(struct regs *r) {
    if (r->int_no >= 40) outb(0xA0, 0x20);
    outb(0x20, 0x20);

    if (r->int_no == 32) {
        scheduler_tick(r);  // Timer for scheduling/counter
    } else if (r->int_no == 33) {
        keyboard_handler();
    }
}
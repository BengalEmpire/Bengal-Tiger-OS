#include "common.h"
#include "keyboard.h"
#include "scheduler.h"

struct regs {
    uint32_t gs, fs, es, ds;
    uint32_t edi, esi, ebp, esp, ebx, edx, ecx, eax;
    uint32_t int_no, err_code;
    uint32_t eip, cs, eflags, useresp, ss;
};

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
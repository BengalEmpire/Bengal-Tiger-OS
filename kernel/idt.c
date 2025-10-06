#include "idt.h"
#include "common.h"

struct idt_entry idt[IDT_ENTRIES];
struct idt_ptr idtp;

extern void isr0(void); // From isr.s
// ... Declare isr1 to isr31, irq0 to irq15 (add as needed)

void idt_set_gate(uint8_t num, uint32_t base, uint16_t sel, uint8_t flags) {
    idt[num].base_low = base & 0xFFFF;
    idt[num].base_high = (base >> 16) & 0xFFFF;
    idt[num].sel = sel;
    idt[num].always0 = 0;
    idt[num].flags = flags;
}

void idt_install() {
    idtp.limit = (sizeof(struct idt_entry) * IDT_ENTRIES) - 1;
    idtp.base = (uint32_t)&idt;
    memset(&idt, 0, sizeof(struct idt_entry) * IDT_ENTRIES);

    // Set exceptions (example for 0, add others)
    idt_set_gate(0, (uint32_t)isr0, 0x08, 0x8E);

    // Set IRQs
    idt_set_gate(32, (uint32_t)irq0, 0x08, 0x8E);
    idt_set_gate(33, (uint32_t)irq1, 0x08, 0x8E);
    // Add more IRQs as needed

    idt_load((uint32_t)&idtp);
}
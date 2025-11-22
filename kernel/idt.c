#include "idt.h"
#include "common.h"

// The IDT Table
struct idt_entry idt[IDT_ENTRIES];
struct idt_ptr idtp;

// CPU Exceptions (0-31)
extern void isr0();  // Divide by Zero
extern void isr1();  // Debug
extern void isr2();  // NMI
extern void isr3();  // Breakpoint
extern void isr4();  // Overflow
extern void isr5();  // Bounds
extern void isr6();  // Invalid Opcode
extern void isr7();  // Device Not Available
extern void isr8();  // Double Fault
extern void isr9();  // Segment Overrun
extern void isr10(); // Invalid TSS
extern void isr11(); // Segment Not Present
extern void isr12(); // Stack Fault
extern void isr13(); // General Protection Fault (GPF)
extern void isr14(); // Page Fault
extern void isr15(); // Reserved


// Hardware Interrupts (IRQs)
extern void irq0();  // Timer
extern void irq1();  // Keyboard
extern void irq2();  // Cascade (used internally)
extern void irq3();  // COM2
extern void irq4();  // COM1
extern void irq5();  // LPT2
extern void irq6();  // Floppy Disk
extern void irq7();  // LPT1

extern void irq8();  // CMOS Real-time Clock
extern void irq9();  // Free
extern void irq10(); // Free
extern void irq11(); // Free
extern void irq12(); // PS/2 Mouse
extern void irq13(); // FPU
extern void irq14(); // Primary ATA Hard Disk
extern void irq15(); // Secondary ATA Hard Disk

// System Call (for later)
extern void isr128(); // 0x80 System Call

void idt_set_gate(uint8_t num, uint32_t base, uint16_t sel, uint8_t flags) {
    idt[num].base_low = base & 0xFFFF;
    idt[num].base_high = (base >> 16) & 0xFFFF;
    idt[num].sel = sel;
    idt[num].always0 = 0;
    idt[num].flags = flags; // 0x8E = Kernel, 0xEE = User
}

void idt_install() {
    idtp.limit = (sizeof(struct idt_entry) * IDT_ENTRIES) - 1;
    idtp.base = (uint32_t)&idt;
    
    // 1. Clear IDT memory
    memset(&idt, 0, sizeof(struct idt_entry) * IDT_ENTRIES);

    // 2. Install CPU Exception Handlers (0-31)
    // 0x08 is Kernel Code Segment, 0x8E is Interrupt Gate (Ring 0)
    idt_set_gate(0, (uint32_t)isr0, 0x08, 0x8E);
    idt_set_gate(1, (uint32_t)isr1, 0x08, 0x8E);
    idt_set_gate(2, (uint32_t)isr2, 0x08, 0x8E);
    idt_set_gate(3, (uint32_t)isr3, 0x08, 0x8E);
    idt_set_gate(4, (uint32_t)isr4, 0x08, 0x8E);
    idt_set_gate(5, (uint32_t)isr5, 0x08, 0x8E);
    idt_set_gate(6, (uint32_t)isr6, 0x08, 0x8E);
    idt_set_gate(7, (uint32_t)isr7, 0x08, 0x8E);
    idt_set_gate(8, (uint32_t)isr8, 0x08, 0x8E); 
    // ... Add 9-12 here ...
    idt_set_gate(13, (uint32_t)isr13, 0x08, 0x8E);
    idt_set_gate(14, (uint32_t)isr14, 0x08, 0x8E);

    // 3. Install Master PIC IRQs (32-39)
    idt_set_gate(32, (uint32_t)irq0, 0x08, 0x8E);
    idt_set_gate(33, (uint32_t)irq1, 0x08, 0x8E);
    idt_set_gate(34, (uint32_t)irq2, 0x08, 0x8E);
    idt_set_gate(35, (uint32_t)irq3, 0x08, 0x8E);
    idt_set_gate(36, (uint32_t)irq4, 0x08, 0x8E);
    idt_set_gate(37, (uint32_t)irq5, 0x08, 0x8E);
    idt_set_gate(38, (uint32_t)irq6, 0x08, 0x8E);
    idt_set_gate(39, (uint32_t)irq7, 0x08, 0x8E);

    // 4. Install Slave PIC IRQs (40-47)
    idt_set_gate(40, (uint32_t)irq8, 0x08, 0x8E);
    idt_set_gate(41, (uint32_t)irq9, 0x08, 0x8E);
    idt_set_gate(42, (uint32_t)irq10, 0x08, 0x8E);
    idt_set_gate(43, (uint32_t)irq11, 0x08, 0x8E);
    idt_set_gate(44, (uint32_t)irq12, 0x08, 0x8E);
    idt_set_gate(45, (uint32_t)irq13, 0x08, 0x8E);
    idt_set_gate(46, (uint32_t)irq14, 0x08, 0x8E);
    idt_set_gate(47, (uint32_t)irq15, 0x08, 0x8E);

    // 5. System Call Gate (0x80)
    // Note: 0xEE allows Ring 3 (User mode) to call this!
    idt_set_gate(0x80, (uint32_t)isr128, 0x08, 0xEE);

    // Load IDT
    idt_load((uint32_t)&idtp);
}
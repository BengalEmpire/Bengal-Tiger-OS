/**
 * Bengal Tiger OS - Interrupt Descriptor Table
 * 
 * Configures the x86 IDT for handling CPU exceptions and IRQs.
 * 
 * @file idt.h
 * @author Bengal Tiger OS (BengalEmpire)
 * @version 0.3.0
 */

#ifndef IDT_H
#define IDT_H

#include "common.h"

/* Number of IDT entries */
#define IDT_ENTRIES 256

/* Gate Types */
#define IDT_GATE_INT    0x8E    /* 32-bit Interrupt Gate, Ring 0 */
#define IDT_GATE_TRAP   0x8F    /* 32-bit Trap Gate, Ring 0 */
#define IDT_GATE_USER   0xEE    /* 32-bit Interrupt Gate, Ring 3 */

/**
 * IDT Entry Structure
 * Each entry is 8 bytes, packed.
 */
struct idt_entry {
    uint16_t base_low;      /* Lower 16 bits of handler address */
    uint16_t sel;           /* Kernel segment selector */
    uint8_t  always0;       /* Reserved, always 0 */
    uint8_t  flags;         /* Type and attributes */
    uint16_t base_high;     /* Upper 16 bits of handler address */
} __attribute__((packed));

/**
 * IDT Pointer Structure
 * Loaded into IDTR register.
 */
struct idt_ptr {
    uint16_t limit;         /* Size of IDT - 1 */
    uint32_t base;          /* Address of IDT */
} __attribute__((packed));

/* External ISR declarations (from isr.s) */

/* CPU Exceptions (0-31) */
extern void isr0(void);     /* Division By Zero */
extern void isr1(void);     /* Debug */
extern void isr2(void);     /* Non Maskable Interrupt */
extern void isr3(void);     /* Breakpoint */
extern void isr4(void);     /* Overflow */
extern void isr5(void);     /* Bound Range Exceeded */
extern void isr6(void);     /* Invalid Opcode */
extern void isr7(void);     /* Device Not Available */
extern void isr8(void);     /* Double Fault */
extern void isr9(void);     /* Coprocessor Segment Overrun */
extern void isr10(void);    /* Invalid TSS */
extern void isr11(void);    /* Segment Not Present */
extern void isr12(void);    /* Stack-Segment Fault */
extern void isr13(void);    /* General Protection Fault */
extern void isr14(void);    /* Page Fault */
extern void isr15(void);    /* Reserved */
extern void isr16(void);    /* x87 FPU Error */
extern void isr17(void);    /* Alignment Check */
extern void isr18(void);    /* Machine Check */
extern void isr19(void);    /* SIMD Floating-Point */
extern void isr20(void);
extern void isr21(void);
extern void isr22(void);
extern void isr23(void);
extern void isr24(void);
extern void isr25(void);
extern void isr26(void);
extern void isr27(void);
extern void isr28(void);
extern void isr29(void);
extern void isr30(void);
extern void isr31(void);

/* Hardware IRQs (remapped to 32-47) */
extern void irq0(void);     /* Timer */
extern void irq1(void);     /* Keyboard */
extern void irq2(void);     /* Cascade */
extern void irq3(void);     /* COM2 */
extern void irq4(void);     /* COM1 */
extern void irq5(void);     /* LPT2 */
extern void irq6(void);     /* Floppy */
extern void irq7(void);     /* LPT1 / Spurious */
extern void irq8(void);     /* CMOS RTC */
extern void irq9(void);     /* Free / ACPI */
extern void irq10(void);    /* Free */
extern void irq11(void);    /* Free */
extern void irq12(void);    /* PS/2 Mouse */
extern void irq13(void);    /* FPU */
extern void irq14(void);    /* Primary ATA */
extern void irq15(void);    /* Secondary ATA */

/* System Call (INT 0x80) */
extern void isr128(void);

/* IDT load function (from isr.s) */
extern void idt_load(uint32_t idt_ptr);

/**
 * Set an entry in the IDT.
 * 
 * @param num Interrupt number (0-255)
 * @param base Address of handler function
 * @param sel Code segment selector (0x08 for kernel)
 * @param flags Gate type and privilege level
 */
void idt_set_gate(uint8_t num, uint32_t base, uint16_t sel, uint8_t flags);

/**
 * Install the IDT.
 * Configures all entries and loads IDTR.
 */
void idt_install(void);

#endif /* IDT_H */
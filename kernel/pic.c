/**
 * Bengal Tiger OS - 8259 PIC Implementation
 *
 * @file pic.c
 * @author Bengal Tiger OS (BengalEmpire)
 * @version 1.0.0
 */

#include "pic.h"
#include "common.h"

static inline void io_wait(void) {
    outb(0x80, 0);
}

void pic_remap(void) {
    outb(PIC1_COMMAND, ICW1_INIT | ICW1_ICW4);
    io_wait();
    outb(PIC2_COMMAND, ICW1_INIT | ICW1_ICW4);
    io_wait();

    outb(PIC1_DATA, 0x20);  /* Master: INT 32-39 */
    io_wait();
    outb(PIC2_DATA, 0x28);  /* Slave:  INT 40-47 */
    io_wait();

    outb(PIC1_DATA, 0x04);  /* Slave on IRQ2 */
    io_wait();
    outb(PIC2_DATA, 0x02);  /* Cascade identity */
    io_wait();

    outb(PIC1_DATA, ICW4_8086);
    io_wait();
    outb(PIC2_DATA, ICW4_8086);
    io_wait();

    outb(PIC1_DATA, 0x00);
    outb(PIC2_DATA, 0x00);
}

void pic_send_eoi(uint8_t irq) {
    if (irq >= 8) {
        outb(PIC2_COMMAND, PIC_EOI);
    }
    outb(PIC1_COMMAND, PIC_EOI);
}

void pic_mask_irq(uint8_t irq) {
    uint16_t port = (irq < 8) ? PIC1_DATA : PIC2_DATA;
    if (irq >= 8) irq -= 8;
    outb(port, (uint8_t)(inb(port) | (1 << irq)));
}

void pic_unmask_irq(uint8_t irq) {
    uint16_t port = (irq < 8) ? PIC1_DATA : PIC2_DATA;
    if (irq >= 8) irq -= 8;
    outb(port, (uint8_t)(inb(port) & ~(1 << irq)));
}

uint16_t pic_get_mask(void) {
    return (uint16_t)inb(PIC1_DATA) | ((uint16_t)inb(PIC2_DATA) << 8);
}

void pic_set_mask(uint16_t mask) {
    outb(PIC1_DATA, (uint8_t)(mask & 0xFF));
    outb(PIC2_DATA, (uint8_t)((mask >> 8) & 0xFF));
}

int pic_is_spurious_irq7(void) {
    outb(PIC1_COMMAND, PIC_READ_ISR);
    uint8_t isr = inb(PIC1_COMMAND);
    return !(isr & 0x80);
}

int pic_is_spurious_irq15(void) {
    outb(PIC2_COMMAND, PIC_READ_ISR);
    uint8_t isr = inb(PIC2_COMMAND);
    return !(isr & 0x80);
}

void pic_disable_all(void) {
    outb(PIC1_DATA, 0xFF);
    outb(PIC2_DATA, 0xFF);
}

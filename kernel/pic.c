/**
 * Bengal Tiger OS - 8259 PIC Implementation
 * 
 * @file pic.c
 * @author Bengal Tiger OS (BengalEmpire)
 * @version 0.3.0
 */

#include "pic.h"
#include "common.h"

/* I/O wait - gives slower devices time to respond */
static inline void io_wait(void) {
    /* Port 0x80 is used for POST codes, writes are safe */
    outb(0x80, 0);
}

void pic_remap(void) {
    /* Save current masks */
    uint8_t mask1 = inb(PIC1_DATA);
    uint8_t mask2 = inb(PIC2_DATA);
    (void)mask1;
    (void)mask2;
    
    /* ICW1: Start initialization sequence */
    outb(PIC1_COMMAND, ICW1_INIT | ICW1_ICW4);
    io_wait();
    outb(PIC2_COMMAND, ICW1_INIT | ICW1_ICW4);
    io_wait();
    
    /* ICW2: Vector offsets */
    outb(PIC1_DATA, 0x20);  /* Master: offset 0x20 (32) */
    io_wait();
    outb(PIC2_DATA, 0x28);  /* Slave: offset 0x28 (40) */
    io_wait();
    
    /* ICW3: Cascade configuration */
    outb(PIC1_DATA, 0x04);  /* Master: slave on IRQ2 */
    io_wait();
    outb(PIC2_DATA, 0x02);  /* Slave: cascade identity */
    io_wait();
    
    /* ICW4: Environment info */
    outb(PIC1_DATA, ICW4_8086);
    io_wait();
    outb(PIC2_DATA, ICW4_8086);
    io_wait();
    
    /* Restore saved masks (or set to 0 to enable all) */
    outb(PIC1_DATA, 0x00);  /* Enable all IRQs on master */
    outb(PIC2_DATA, 0x00);  /* Enable all IRQs on slave */
}

void pic_send_eoi(uint8_t irq) {
    /* If IRQ is from slave PIC (8-15), send EOI to both */
    if (irq >= 8) {
        outb(PIC2_COMMAND, PIC_EOI);
    }
    outb(PIC1_COMMAND, PIC_EOI);
}

void pic_mask_irq(uint8_t irq) {
    uint16_t port;
    uint8_t value;
    
    if (irq < 8) {
        port = PIC1_DATA;
    } else {
        port = PIC2_DATA;
        irq -= 8;
    }
    
    value = inb(port) | (1 << irq);
    outb(port, value);
}

void pic_unmask_irq(uint8_t irq) {
    uint16_t port;
    uint8_t value;
    
    if (irq < 8) {
        port = PIC1_DATA;
    } else {
        port = PIC2_DATA;
        irq -= 8;
    }
    
    value = inb(port) & ~(1 << irq);
    outb(port, value);
}

uint16_t pic_get_mask(void) {
    return (uint16_t)inb(PIC1_DATA) | ((uint16_t)inb(PIC2_DATA) << 8);
}

void pic_set_mask(uint16_t mask) {
    outb(PIC1_DATA, mask & 0xFF);
    outb(PIC2_DATA, (mask >> 8) & 0xFF);
}

int pic_is_spurious_irq7(void) {
    /* Read In-Service Register from master PIC */
    outb(PIC1_COMMAND, PIC_READ_ISR);
    uint8_t isr = inb(PIC1_COMMAND);
    
    /* If bit 7 is not set, it's a spurious interrupt */
    return !(isr & 0x80);
}

void pic_disable_all(void) {
    outb(PIC1_DATA, 0xFF);
    outb(PIC2_DATA, 0xFF);
}
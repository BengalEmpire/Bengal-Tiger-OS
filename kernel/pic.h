/**
 * Bengal Tiger OS - 8259 Programmable Interrupt Controller
 *
 * @file pic.h
 * @author Bengal Tiger OS (BengalEmpire)
 * @version 1.0.0
 */

#ifndef PIC_H
#define PIC_H

#include "common.h"

/* PIC I/O Ports */
#define PIC1_COMMAND    0x20
#define PIC1_DATA       0x21
#define PIC2_COMMAND    0xA0
#define PIC2_DATA       0xA1

/* PIC Commands */
#define PIC_EOI         0x20
#define PIC_READ_ISR    0x0B
#define PIC_READ_IRR    0x0A

/* ICW1 & ICW4 */
#define ICW1_ICW4       0x01
#define ICW1_INIT       0x10
#define ICW4_8086       0x01

void pic_remap(void);
void pic_send_eoi(uint8_t irq);
void pic_mask_irq(uint8_t irq);
void pic_unmask_irq(uint8_t irq);
uint16_t pic_get_mask(void);
void pic_set_mask(uint16_t mask);
int pic_is_spurious_irq7(void);
int pic_is_spurious_irq15(void);
void pic_disable_all(void);

#endif /* PIC_H */

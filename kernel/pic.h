/**
 * Bengal Tiger OS - 8259 Programmable Interrupt Controller
 * 
 * Handles the two 8259A PICs (master and slave) that manage
 * hardware interrupt routing to the CPU.
 * 
 * @file pic.h
 * @author Bengal Tiger OS (BengalEmpire)
 * @version 0.3.0
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
#define PIC_EOI         0x20    /* End of Interrupt */
#define PIC_READ_ISR    0x0B    /* Read In-Service Register */
#define PIC_READ_IRR    0x0A    /* Read Interrupt Request Register */

/* ICW1 (Initialization Command Word 1) */
#define ICW1_ICW4       0x01    /* ICW4 needed */
#define ICW1_INIT       0x10    /* Initialization */

/* ICW4 (Initialization Command Word 4) */
#define ICW4_8086       0x01    /* 8086/88 mode */

/**
 * Remap PIC interrupt vectors.
 * 
 * By default, IRQ 0-7 use INT 0x08-0x0F (conflicts with CPU exceptions!)
 * This remaps them to:
 *   - Master PIC (IRQ 0-7)  → INT 0x20-0x27 (32-39)
 *   - Slave PIC  (IRQ 8-15) → INT 0x28-0x2F (40-47)
 */
void pic_remap(void);

/**
 * Send End of Interrupt signal to PIC(s).
 * Must be called at the end of every IRQ handler.
 * 
 * @param irq IRQ number (0-15)
 */
void pic_send_eoi(uint8_t irq);

/**
 * Mask (disable) a specific IRQ.
 * 
 * @param irq IRQ number (0-15)
 */
void pic_mask_irq(uint8_t irq);

/**
 * Unmask (enable) a specific IRQ.
 * 
 * @param irq IRQ number (0-15)
 */
void pic_unmask_irq(uint8_t irq);

/**
 * Get the current interrupt mask.
 * 
 * @return 16-bit mask (bit set = IRQ disabled)
 */
uint16_t pic_get_mask(void);

/**
 * Set the interrupt mask.
 * 
 * @param mask 16-bit mask (bit set = IRQ disabled)
 */
void pic_set_mask(uint16_t mask);

/**
 * Check for spurious IRQ 7 (common issue with PIC).
 * 
 * @return 1 if spurious, 0 if real
 */
int pic_is_spurious_irq7(void);

/**
 * Disable all IRQs (mask all).
 */
void pic_disable_all(void);

#endif /* PIC_H */
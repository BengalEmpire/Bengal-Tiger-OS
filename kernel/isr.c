/**
 * Bengal Tiger OS - Interrupt & System Call Handlers
 *
 * C-level dispatchers invoked by assembly stubs in isr.s.
 *
 * @file isr.c
 * @author Bengal Tiger OS (BengalEmpire)
 * @version 1.0.0
 */

#include "common.h"
#include "pic.h"
#include "keyboard.h"
#include "timer.h"
#include "panic.h"
#include "mouse.h"
#include "scheduler.h"
#include "serial.h"
#include "shell.h"
#include "nic.h"

/* System Call Numbers (INT 0x80 ABI) */
#define SYS_EXIT    1
#define SYS_WRITE   4
#define SYS_GETPID  20
#define SYS_SLEEP   35
#define SYS_UPTIME  99
#define SYS_YIELD   158

struct regs* syscall_handler(struct regs *r) {
    uint32_t syscall_num = r->eax;

    switch (syscall_num) {
        case SYS_EXIT: {
            task_t *cur = scheduler_get_current();
            if (cur && cur->id != 0) {
                cur->state = TASK_STATE_ZOMBIE;
                return scheduler_schedule(r);
            }
            r->eax = (uint32_t)-1;
            break;
        }
        case SYS_WRITE: {
            /* ebx = fd, ecx = buf, edx = len */
            const char *buf = (const char*)r->ecx;
            uint32_t len = r->edx;
            if (buf && len > 0 && len <= 4096) {
                for (uint32_t i = 0; i < len; i++) {
                    char tmp[2] = {buf[i], '\0'};
                    shell_print(tmp);
                }
                r->eax = len;
            } else {
                r->eax = (uint32_t)-1;
            }
            break;
        }
        case SYS_GETPID: {
            task_t *cur = scheduler_get_current();
            r->eax = cur ? cur->id : 0;
            break;
        }
        case SYS_SLEEP:
            sleep_ms(r->ebx);
            r->eax = 0;
            break;
        case SYS_UPTIME:
            r->eax = timer_get_ms();
            break;
        case SYS_YIELD:
            r->eax = 0;
            return scheduler_schedule(r);
        default:
            r->eax = (uint32_t)-1;
            break;
    }

    return r;
}

/**
 * CPU Exception & Software Interrupt Handler
 */
struct regs* isr_handler(struct regs *r) {
    if (r->int_no == 0x80) {
        return syscall_handler(r);
    }
    exception_handler(r);
    return r;
}

/**
 * Hardware IRQ Handler (vectors 32-47)
 */
struct regs* irq_handler(struct regs *r) {
    uint8_t irq = (uint8_t)(r->int_no - 32);

    /* Guard against 8259 spurious IRQ7 and IRQ15 on real hardware */
    if (irq == 7 && pic_is_spurious_irq7()) {
        return r;
    }
    if (irq == 15 && pic_is_spurious_irq15()) {
        outb(PIC1_COMMAND, PIC_EOI); /* Master PIC still needs EOI for cascade */
        return r;
    }

    /* Send EOI to PIC(s) */
    pic_send_eoi(irq);

    struct regs *ret_r = r;

    switch (r->int_no) {
        case 32:  /* IRQ0 - PIT Timer */
            timer_handler(r);
            ret_r = scheduler_tick(r);
            break;

        case 33:  /* IRQ1 - PS/2 Keyboard */
            keyboard_handler();
            break;

        case 36:  /* IRQ4 - COM1 Serial RX */
            serial_irq_handler();
            break;

        case 44:  /* IRQ12 - PS/2 Mouse */
            mouse_handler();
            break;

        default:
            break;
    }

    /* Acknowledge PCI NIC interrupt if shared or assigned to this IRQ */
    nic_handle_irq(irq);

    return ret_r;
}

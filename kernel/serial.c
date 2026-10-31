/**
 * Bengal Tiger OS - 16550 UART Serial Port Driver Implementation
 *
 * @file serial.c
 * @author Bengal Tiger OS (BengalEmpire)
 * @version 1.0.0
 */

#include "serial.h"
#include "pic.h"
#include "common.h"

#define SERIAL_RX_RING_SIZE 256

static uint16_t default_port = 0;
static int serial_present = 0;

static char serial_rx_ring[SERIAL_RX_RING_SIZE];
static volatile uint16_t srx_head = 0;
static volatile uint16_t srx_tail = 0;

static void serial_rx_push(char c) {
    uint16_t next = (uint16_t)((srx_head + 1) & (SERIAL_RX_RING_SIZE - 1));
    if (next != srx_tail) {
        serial_rx_ring[srx_head] = c;
        srx_head = next;
    }
}

void serial_init(uint16_t port, uint32_t baud) {
    if (default_port == 0) {
        default_port = port;
    }

    uint16_t divisor = (baud > 0) ? (uint16_t)(115200U / baud) : 1;
    if (divisor == 0) divisor = 1;

    outb(port + SERIAL_INTR_ENABLE, 0x00);
    outb(port + SERIAL_LINE_CTRL, SERIAL_LCR_DLAB);
    outb(port + SERIAL_DLL, (uint8_t)(divisor & 0xFF));
    outb(port + SERIAL_DLM, (uint8_t)((divisor >> 8) & 0xFF));
    outb(port + SERIAL_LINE_CTRL, SERIAL_LCR_8BIT);
    outb(port + SERIAL_FIFO_CTRL,
         SERIAL_FCR_ENABLE | SERIAL_FCR_CLEAR_RX |
         SERIAL_FCR_CLEAR_TX | SERIAL_FCR_TRIG_14);
    outb(port + SERIAL_MODEM_CTRL,
         SERIAL_MCR_DTR | SERIAL_MCR_RTS | SERIAL_MCR_OUT2);

    /* Scratch register test to detect floating 0xFF bus when UART absent */
    outb(port + SERIAL_SCRATCH, 0x5A);
    if (inb(port + SERIAL_SCRATCH) == 0x5A) {
        serial_present = 1;
        /* Drain stale RX bytes and enable Receiver Data Available interrupt */
        while (inb(port + SERIAL_LINE_STAT) & SERIAL_LSR_DR) {
            (void)inb(port + SERIAL_DATA);
        }
        outb(port + SERIAL_INTR_ENABLE, 0x01);
    } else {
        /* Still allow QEMU isa-debugcon/simple UARTs that omit scratch */
        serial_present = (inb(port + SERIAL_LINE_STAT) != 0xFF) ? 1 : 0;
        if (serial_present) {
            outb(port + SERIAL_INTR_ENABLE, 0x01);
        }
    }
}

void serial_write_char(uint16_t port, char c) {
    if (!serial_present && port == default_port) return;

    if (c == '\n') {
        serial_write_char(port, '\r');
    }

    int timeout = 100000;
    while (timeout-- > 0 && !(inb(port + SERIAL_LINE_STAT) & SERIAL_LSR_THRE)) {
        __asm__ volatile("pause");
    }

    outb(port + SERIAL_DATA, (uint8_t)c);
}

void serial_write_str_port(uint16_t port, const char *str) {
    if (!str) return;
    while (*str) {
        serial_write_char(port, *str++);
    }
}

void serial_write_str(const char *str) {
    if (default_port != 0 && serial_present) {
        serial_write_str_port(default_port, str);
    }
}

int serial_has_data(uint16_t port) {
    if (!serial_present && port == default_port) return 0;
    uint8_t lsr = inb(port + SERIAL_LINE_STAT);
    if (lsr == 0xFF) return 0;
    return (lsr & SERIAL_LSR_DR) ? 1 : 0;
}

char serial_read_char(uint16_t port) {
    while (!serial_has_data(port)) {
        __asm__ volatile("pause");
    }
    return (char)inb(port + SERIAL_DATA);
}

void serial_poll_rx(void) {
    if (!default_port || !serial_present) return;
    uint32_t flags = irq_save();
    while (serial_has_data(default_port)) {
        char c = (char)inb(default_port + SERIAL_DATA);
        serial_rx_push(c);
    }
    irq_restore(flags);
}

void serial_irq_handler(void) {
    if (!default_port || !serial_present) return;
    while (serial_has_data(default_port)) {
        char c = (char)inb(default_port + SERIAL_DATA);
        serial_rx_push(c);
    }
}

int serial_pop_rx_char(char *out_ch) {
    serial_poll_rx();
    uint32_t flags = irq_save();
    if (srx_head == srx_tail) {
        irq_restore(flags);
        return 0;
    }
    *out_ch = serial_rx_ring[srx_tail];
    srx_tail = (uint16_t)((srx_tail + 1) & (SERIAL_RX_RING_SIZE - 1));
    irq_restore(flags);
    return 1;
}

void serial_write_hex(uint16_t port, uint32_t val) {
    const char *hex_digits = "0123456789ABCDEF";
    char buf[11] = "0x00000000";
    for (int i = 9; i >= 2; i--) {
        buf[i] = hex_digits[val & 0x0F];
        val >>= 4;
    }
    serial_write_str_port(port, buf);
}

void serial_write_int(uint16_t port, int32_t val) {
    char buf[16];
    k_itoa(val, buf, 10);
    serial_write_str_port(port, buf);
}

/**
 * Bengal Tiger OS - Serial Port Driver Implementation
 *
 * @file serial.c
 * @author Bengal Tiger OS (BengalEmpire)
 * @version 0.4.0
 */

#include "serial.h"
#include "common.h"

/* Default serial port for quick debug output */
static uint16_t default_port = 0;

void serial_init(uint16_t port, uint32_t baud) {
    /* Save as default if first initialization */
    if (default_port == 0) {
        default_port = port;
    }

    /* Calculate divisor from baud rate */
    uint16_t divisor;
    if (baud > 0) {
        divisor = (uint16_t)(115200 / baud);
    } else {
        divisor = 1; /* 115200 baud */
    }

    /* Disable interrupts */
    outb(port + SERIAL_INTR_ENABLE, 0x00);

    /* Set DLAB to access divisor latch */
    outb(port + SERIAL_LINE_CTRL, SERIAL_LCR_DLAB);

    /* Set baud rate divisor */
    outb(port + SERIAL_DLL, divisor & 0xFF);
    outb(port + SERIAL_DLM, (divisor >> 8) & 0xFF);

    /* 8-bit data, no parity, 1 stop bit */
    outb(port + SERIAL_LINE_CTRL, SERIAL_LCR_8BIT);

    /* Enable FIFO, clear them, with 14-byte threshold */
    outb(port + SERIAL_FIFO_CTRL,
         SERIAL_FCR_ENABLE | SERIAL_FCR_CLEAR_RX |
         SERIAL_FCR_CLEAR_TX | SERIAL_FCR_TRIG_14);

    /* Enable DTR, RTS, and OUT2 (for IRQ) */
    outb(port + SERIAL_MODEM_CTRL,
         SERIAL_MCR_DTR | SERIAL_MCR_RTS | SERIAL_MCR_OUT2);

    /* Small delay for UART to stabilize */
    inb(port + SERIAL_LINE_STAT);
}

void serial_write_char(uint16_t port, char c) {
    /* Handle newline expansion: LF -> CR+LF */
    if (c == '\n') {
        serial_write_char(port, '\r');
    }

    /* Wait for transmitter holding register to be empty */
    while (!(inb(port + SERIAL_LINE_STAT) & SERIAL_LSR_THRE)) {
        __asm__ volatile("pause");
    }

    outb(port + SERIAL_DATA, (uint8_t)c);
}

void serial_write_str_port(uint16_t port, const char *str) {
    if (!str) return;

    while (*str) {
        serial_write_char(port, *str);
        str++;
    }
}

void serial_write_str(const char *str) {
    if (default_port != 0) {
        serial_write_str_port(default_port, str);
    }
}

char serial_read_char(uint16_t port) {
    /* Wait for data to be available */
    while (!(inb(port + SERIAL_LINE_STAT) & SERIAL_LSR_DR)) {
        __asm__ volatile("pause");
    }

    return (char)inb(port + SERIAL_DATA);
}

int serial_has_data(uint16_t port) {
    return (inb(port + SERIAL_LINE_STAT) & SERIAL_LSR_DR) ? 1 : 0;
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
    int pos = 0;

    if (val < 0) {
        serial_write_char(port, '-');
        val = -val;
    }

    if (val == 0) {
        serial_write_char(port, '0');
        return;
    }

    while (val > 0 && pos < 15) {
        buf[pos++] = '0' + (val % 10);
        val /= 10;
    }

    /* Reverse and print */
    while (pos > 0) {
        serial_write_char(port, buf[--pos]);
    }
}

/**
 * Bengal Tiger OS - Serial Port Driver
 *
 * Provides serial (RS-232) communication via standard UART 16550.
 * Primarily used for kernel debug output on real hardware.
 *
 * @file serial.h
 * @author Bengal Tiger OS (BengalEmpire)
 * @version 0.4.0
 */

#ifndef SERIAL_H
#define SERIAL_H

#include "common.h"

/* Standard COM Port I/O Addresses */
#define COM1_PORT   0x3F8
#define COM2_PORT   0x2F8
#define COM3_PORT   0x3E8
#define COM4_PORT   0x2E8

/* UART Register Offsets (with DLAB = 0 unless noted) */
#define SERIAL_DATA         0   /* Data register (R/W) */
#define SERIAL_INTR_ENABLE  1   /* Interrupt Enable Register */
#define SERIAL_FIFO_CTRL    2   /* FIFO Control Register (write only) */
#define SERIAL_LINE_CTRL    3   /* Line Control Register */
#define SERIAL_MODEM_CTRL   4   /* Modem Control Register */
#define SERIAL_LINE_STAT    5   /* Line Status Register */
#define SERIAL_MODEM_STAT   6   /* Modem Status Register */
#define SERIAL_SCRATCH      7   /* Scratch register */

/* Divisor Latch (when DLAB=1, at offsets 0 and 1) */
#define SERIAL_DLL          0   /* Divisor latch low byte */
#define SERIAL_DLM          1   /* Divisor latch high byte */

/* Line Control Register bits */
#define SERIAL_LCR_5BIT     0x00   /* 5-bit word length */
#define SERIAL_LCR_6BIT     0x01   /* 6-bit */
#define SERIAL_LCR_7BIT     0x02   /* 7-bit */
#define SERIAL_LCR_8BIT     0x03   /* 8-bit */
#define SERIAL_LCR_2STOP    0x04   /* 2 stop bits */
#define SERIAL_LCR_PAREN    0x08   /* Parity enable */
#define SERIAL_LCR_EVEN     0x10   /* Even parity (0 = odd) */
#define SERIAL_LCR_STICK    0x20   /* Stick parity */
#define SERIAL_LCR_BREAK    0x40   /* Break signal */
#define SERIAL_LCR_DLAB     0x80   /* Divisor Latch Access Bit */

/* Line Status Register bits */
#define SERIAL_LSR_DR       0x01   /* Data Ready */
#define SERIAL_LSR_OE       0x02   /* Overrun Error */
#define SERIAL_LSR_PE       0x04   /* Parity Error */
#define SERIAL_LSR_FE       0x08   /* Framing Error */
#define SERIAL_LSR_BI       0x10   /* Break Interrupt */
#define SERIAL_LSR_THRE     0x20   /* Transmitter Holding Register Empty */
#define SERIAL_LSR_TEMT     0x40   /* Transmitter Empty */
#define SERIAL_LSR_ERR      0x80   /* Error in FIFO */

/* FIFO Control Register bits */
#define SERIAL_FCR_ENABLE   0x01   /* Enable FIFOs */
#define SERIAL_FCR_CLEAR_RX 0x02   /* Clear Receive FIFO */
#define SERIAL_FCR_CLEAR_TX 0x04   /* Clear Transmit FIFO */
#define SERIAL_FCR_TRIG_1   0x00   /* Trigger at 1 byte */
#define SERIAL_FCR_TRIG_4   0x40   /* Trigger at 4 bytes */
#define SERIAL_FCR_TRIG_8   0x80   /* Trigger at 8 bytes */
#define SERIAL_FCR_TRIG_14  0xC0   /* Trigger at 14 bytes */

/* Modem Control Register bits */
#define SERIAL_MCR_DTR      0x01   /* Data Terminal Ready */
#define SERIAL_MCR_RTS      0x02   /* Request To Send */
#define SERIAL_MCR_OUT1     0x04   /* Auxiliary output 1 */
#define SERIAL_MCR_OUT2     0x08   /* Auxiliary output 2 (IRQ enable) */
#define SERIAL_MCR_LOOP     0x10   /* Loopback mode */

/**
 * Initialize a serial port.
 *
 * @param port   Base I/O address (e.g., COM1_PORT = 0x3F8)
 * @param baud   Baud rate (e.g., 9600, 19200, 38400, 57600, 115200)
 */
void serial_init(uint16_t port, uint32_t baud);

/**
 * Write a single character to serial port.
 * Blocks until transmitter is ready.
 *
 * @param port  Base I/O address
 * @param c     Character to write
 */
void serial_write_char(uint16_t port, char c);

/**
 * Write a null-terminated string to serial port.
 *
 * @param port  Base I/O address
 * @param str   String to write
 */
void serial_write_str_port(uint16_t port, const char *str);

/**
 * Write a string to the default serial port (COM1).
 * Convenience wrapper for quick debug output.
 *
 * @param str   String to write
 */
void serial_write_str(const char *str);

/**
 * Read a character from serial port.
 * Blocks until data is available.
 *
 * @param port  Base I/O address
 * @return Character read
 */
char serial_read_char(uint16_t port);

/**
 * Check if a character is available to read.
 *
 * @param port  Base I/O address
 * @return 1 if data available, 0 otherwise
 */
int serial_has_data(uint16_t port);

/**
 * Write a hex value as human-readable string to serial.
 * Useful for debugging numeric values.
 *
 * @param port  Base I/O address
 * @param val   Value to print in hex
 */
void serial_write_hex(uint16_t port, uint32_t val);

/**
 * Write a decimal integer to serial.
 *
 * @param port  Base I/O address
 * @param val   Value to print
 */
void serial_write_int(uint16_t port, int32_t val);

#endif /* SERIAL_H */

/**
 * Bengal Tiger OS - 16550 UART Serial Port Driver
 *
 * Supports interrupt-driven and polled serial console I/O on COM1-COM4.
 *
 * @file serial.h
 * @author Bengal Tiger OS (BengalEmpire)
 * @version 1.0.0
 */

#ifndef SERIAL_H
#define SERIAL_H

#include "common.h"

#define COM1_PORT   0x3F8
#define COM2_PORT   0x2F8
#define COM3_PORT   0x3E8
#define COM4_PORT   0x2E8

#define SERIAL_DATA         0
#define SERIAL_INTR_ENABLE  1
#define SERIAL_FIFO_CTRL    2
#define SERIAL_LINE_CTRL    3
#define SERIAL_MODEM_CTRL   4
#define SERIAL_LINE_STAT    5
#define SERIAL_MODEM_STAT   6
#define SERIAL_SCRATCH      7

#define SERIAL_DLL          0
#define SERIAL_DLM          1

#define SERIAL_LCR_8BIT     0x03
#define SERIAL_LCR_DLAB     0x80

#define SERIAL_LSR_DR       0x01
#define SERIAL_LSR_THRE     0x20

#define SERIAL_FCR_ENABLE   0x01
#define SERIAL_FCR_CLEAR_RX 0x02
#define SERIAL_FCR_CLEAR_TX 0x04
#define SERIAL_FCR_TRIG_14  0xC0

#define SERIAL_MCR_DTR      0x01
#define SERIAL_MCR_RTS      0x02
#define SERIAL_MCR_OUT2     0x08

void serial_init(uint16_t port, uint32_t baud);
void serial_write_char(uint16_t port, char c);
void serial_write_str_port(uint16_t port, const char *str);
void serial_write_str(const char *str);
char serial_read_char(uint16_t port);
int  serial_has_data(uint16_t port);
void serial_write_hex(uint16_t port, uint32_t val);
void serial_write_int(uint16_t port, int32_t val);

/* Interrupt & non-blocking RX buffer support for interactive serial console */
void serial_irq_handler(void);
void serial_poll_rx(void);
int  serial_pop_rx_char(char *out_ch);

#endif /* SERIAL_H */

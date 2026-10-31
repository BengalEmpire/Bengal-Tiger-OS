/**
 * Bengal Tiger OS - Timer & PC Speaker Subsystem
 *
 * Configures PIT Channel 0 at 100Hz (10ms tick) for preemptive scheduling,
 * uptime accounting, interrupt-safe sleep, and PIT Channel 2 PC speaker tones.
 *
 * @file timer.h
 * @author Bengal Tiger OS (BengalEmpire)
 * @version 1.0.0
 */

#ifndef TIMER_H
#define TIMER_H

#include "common.h"

#define PIT_FREQUENCY       1193182     /* Exact NTSC/PIT crystal frequency in Hz */
#define TIMER_HZ            100         /* 100Hz = 10ms tick */
#define TIMER_DIVISOR       (PIT_FREQUENCY / TIMER_HZ)

/* PIT I/O Ports */
#define PIT_CHANNEL0        0x40
#define PIT_CHANNEL1        0x41
#define PIT_CHANNEL2        0x42
#define PIT_COMMAND         0x43
#define PC_SPEAKER_PORT     0x61

/* PIT Command Byte Flags */
#define PIT_CMD_CHANNEL0    0x00
#define PIT_CMD_CHANNEL2    0x80
#define PIT_CMD_LOBYTE      0x10
#define PIT_CMD_HIBYTE      0x20
#define PIT_CMD_LOHIBYTE    0x30
#define PIT_CMD_MODE3       0x06

/* Timer State Structure (volatile because updated in IRQ0) */
typedef struct {
    volatile uint32_t ticks;
    volatile uint32_t seconds;
    uint32_t frequency;
} timer_state_t;

extern timer_state_t timer_state;

void timer_init(void);
uint32_t timer_get_ticks(void);
uint32_t timer_get_seconds(void);
uint32_t timer_get_ms(void);
void sleep_ms(uint32_t ms);
void sleep_s(uint32_t seconds);
void timer_handler(struct regs *r);
void timer_format_uptime(char *buffer);
void speaker_beep(uint32_t freq_hz, uint32_t duration_ms);

#endif /* TIMER_H */

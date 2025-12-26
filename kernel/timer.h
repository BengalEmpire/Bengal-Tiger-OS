/**
 * Bengal Tiger OS - Timer Subsystem
 * 
 * Provides accurate timing services using the Programmable Interval Timer (PIT).
 * The PIT is configured to generate interrupts at 100Hz (every 10ms).
 * 
 * Features:
 * - System uptime tracking
 * - Millisecond-precision sleep functions
 * - Timer callback registration
 * 
 * @file timer.h
 * @author Bengal Tiger OS (BengalEmpire)
 * @version 0.3.0
 */

#ifndef TIMER_H
#define TIMER_H

#include "common.h"

/* Timer Configuration */
#define PIT_FREQUENCY       1193180     /* Base frequency of PIT in Hz */
#define TIMER_HZ            100         /* Target timer frequency (100Hz = 10ms) */
#define TIMER_DIVISOR       (PIT_FREQUENCY / TIMER_HZ)

/* PIT I/O Ports */
#define PIT_CHANNEL0        0x40
#define PIT_CHANNEL1        0x41
#define PIT_CHANNEL2        0x42
#define PIT_COMMAND         0x43

/* PIT Command Byte Flags */
#define PIT_CMD_CHANNEL0    0x00
#define PIT_CMD_LOBYTE      0x10
#define PIT_CMD_HIBYTE      0x20
#define PIT_CMD_LOHIBYTE    0x30
#define PIT_CMD_MODE3       0x06        /* Square wave generator */

/* Timer State Structure */
typedef struct {
    uint32_t ticks;             /* Total ticks since boot */
    uint32_t seconds;           /* Total seconds since boot */
    uint32_t frequency;         /* Timer frequency in Hz */
} timer_state_t;

/* Global Timer State */
extern timer_state_t timer_state;

/**
 * Initialize the timer subsystem.
 * Configures PIT Channel 0 to generate interrupts at TIMER_HZ.
 */
void timer_init(void);

/**
 * Get total system uptime in ticks.
 * @return Number of ticks since boot
 */
uint32_t timer_get_ticks(void);

/**
 * Get total system uptime in seconds.
 * @return Number of seconds since boot
 */
uint32_t timer_get_seconds(void);

/**
 * Get total system uptime in milliseconds.
 * @return Number of milliseconds since boot (may wrap around)
 */
uint32_t timer_get_ms(void);

/**
 * Sleep for specified milliseconds.
 * Uses HLT instruction to save power while waiting.
 * @param ms Number of milliseconds to sleep
 */
void sleep_ms(uint32_t ms);

/**
 * Sleep for specified seconds.
 * @param seconds Number of seconds to sleep
 */
void sleep_s(uint32_t seconds);

/**
 * Timer interrupt handler - called by IRQ0.
 * Updates tick counter and invokes registered callbacks.
 * @param r Pointer to saved CPU registers
 */
void timer_handler(struct regs *r);

/**
 * Format uptime as human-readable string.
 * @param buffer Output buffer (at least 32 bytes)
 */
void timer_format_uptime(char *buffer);

#endif /* TIMER_H */

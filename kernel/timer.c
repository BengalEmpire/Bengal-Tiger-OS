/**
 * Bengal Tiger OS - Timer Subsystem Implementation
 * 
 * @file timer.c
 * @author Bengal Tiger OS (BengalEmpire)
 * @version 0.3.0
 */

#include "timer.h"
#include "common.h"

/* Global timer state */
timer_state_t timer_state = {0, 0, TIMER_HZ};

/* Track milliseconds accurately */
static uint32_t ms_counter = 0;

void timer_init(void) {
    /* Reset timer state */
    timer_state.ticks = 0;
    timer_state.seconds = 0;
    timer_state.frequency = TIMER_HZ;
    ms_counter = 0;

    /* 
     * Configure PIT Channel 0
     * Command byte: 0x36 = Channel 0 | Low/High byte | Mode 3 (square wave)
     */
    outb(PIT_COMMAND, PIT_CMD_CHANNEL0 | PIT_CMD_LOHIBYTE | PIT_CMD_MODE3);

    /* Send divisor (low byte first, then high byte) */
    uint16_t divisor = TIMER_DIVISOR;
    outb(PIT_CHANNEL0, divisor & 0xFF);         /* Low byte */
    outb(PIT_CHANNEL0, (divisor >> 8) & 0xFF);  /* High byte */
}

uint32_t timer_get_ticks(void) {
    return timer_state.ticks;
}

uint32_t timer_get_seconds(void) {
    return timer_state.seconds;
}

uint32_t timer_get_ms(void) {
    /* Each tick is 10ms at 100Hz */
    return timer_state.ticks * (1000 / TIMER_HZ);
}

void timer_handler(struct regs *r) {
    (void)r;  /* Unused parameter */

    timer_state.ticks++;
    ms_counter += (1000 / TIMER_HZ);  /* Add 10ms per tick */

    /* Update seconds counter */
    if (timer_state.ticks % TIMER_HZ == 0) {
        timer_state.seconds++;
    }
}

void sleep_ms(uint32_t ms) {
    uint32_t start = timer_get_ticks();
    uint32_t ticks_to_wait = ms / (1000 / TIMER_HZ);
    
    if (ticks_to_wait == 0) ticks_to_wait = 1;  /* Minimum 1 tick */

    while ((timer_get_ticks() - start) < ticks_to_wait) {
        /* HLT saves power - CPU wakes on next interrupt */
        __asm__ volatile("hlt");
    }
}

void sleep_s(uint32_t seconds) {
    sleep_ms(seconds * 1000);
}

void timer_format_uptime(char *buffer) {
    uint32_t total_seconds = timer_state.seconds;
    uint32_t days = total_seconds / 86400;
    uint32_t hours = (total_seconds % 86400) / 3600;
    uint32_t minutes = (total_seconds % 3600) / 60;
    uint32_t seconds = total_seconds % 60;

    /* Simple number-to-string formatting */
    int pos = 0;

    if (days > 0) {
        /* Days */
        if (days >= 100) buffer[pos++] = '0' + (days / 100) % 10;
        if (days >= 10) buffer[pos++] = '0' + (days / 10) % 10;
        buffer[pos++] = '0' + days % 10;
        buffer[pos++] = 'd';
        buffer[pos++] = ' ';
    }

    /* Hours */
    buffer[pos++] = '0' + (hours / 10) % 10;
    buffer[pos++] = '0' + hours % 10;
    buffer[pos++] = ':';

    /* Minutes */
    buffer[pos++] = '0' + (minutes / 10) % 10;
    buffer[pos++] = '0' + minutes % 10;
    buffer[pos++] = ':';

    /* Seconds */
    buffer[pos++] = '0' + (seconds / 10) % 10;
    buffer[pos++] = '0' + seconds % 10;

    buffer[pos] = '\0';
}

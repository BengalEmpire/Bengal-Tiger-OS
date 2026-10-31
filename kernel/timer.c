/**
 * Bengal Tiger OS - Timer & PC Speaker Implementation
 *
 * @file timer.c
 * @author Bengal Tiger OS (BengalEmpire)
 * @version 1.0.0
 */

#include "timer.h"
#include "common.h"

timer_state_t timer_state = {0, 0, TIMER_HZ};

void timer_init(void) {
    timer_state.ticks = 0;
    timer_state.seconds = 0;
    timer_state.frequency = TIMER_HZ;

    outb(PIT_COMMAND, PIT_CMD_CHANNEL0 | PIT_CMD_LOHIBYTE | PIT_CMD_MODE3);

    uint16_t divisor = (uint16_t)TIMER_DIVISOR;
    outb(PIT_CHANNEL0, (uint8_t)(divisor & 0xFF));
    outb(PIT_CHANNEL0, (uint8_t)((divisor >> 8) & 0xFF));
}

uint32_t timer_get_ticks(void) {
    return timer_state.ticks;
}

uint32_t timer_get_seconds(void) {
    return timer_state.seconds;
}

uint32_t timer_get_ms(void) {
    return timer_state.ticks * (1000 / TIMER_HZ);
}

void timer_handler(struct regs *r) {
    UNUSED(r);
    uint32_t t = timer_state.ticks + 1;
    timer_state.ticks = t;
    if ((t % TIMER_HZ) == 0) {
        timer_state.seconds = timer_state.seconds + 1;
    }
}

void sleep_ms(uint32_t ms) {
    if (ms == 0) return;

    /* If interrupts are disabled, fall back to port 0x80 I/O delay (~1us/read)
     * so we NEVER deadlock on HLT with IF=0. */
    if (!irq_are_enabled()) {
        for (uint32_t i = 0; i < ms * 1000; i++) {
            inb(0x80);
        }
        return;
    }

    uint32_t start = timer_state.ticks;
    uint32_t ticks_to_wait = ms / (1000 / TIMER_HZ);
    if (ticks_to_wait == 0) ticks_to_wait = 1;

    while ((timer_state.ticks - start) < ticks_to_wait) {
        __asm__ volatile("hlt" ::: "memory");
    }
}

void sleep_s(uint32_t seconds) {
    sleep_ms(seconds * 1000);
}

void speaker_beep(uint32_t freq_hz, uint32_t duration_ms) {
    if (freq_hz < 20 || freq_hz > 20000 || duration_ms == 0) return;

    uint32_t div = PIT_FREQUENCY / freq_hz;
    outb(PIT_COMMAND, PIT_CMD_CHANNEL2 | PIT_CMD_LOHIBYTE | PIT_CMD_MODE3);
    outb(PIT_CHANNEL2, (uint8_t)(div & 0xFF));
    outb(PIT_CHANNEL2, (uint8_t)((div >> 8) & 0xFF));

    uint8_t tmp = inb(PC_SPEAKER_PORT);
    if ((tmp & 0x03) != 0x03) {
        outb(PC_SPEAKER_PORT, (uint8_t)(tmp | 0x03));
    }

    sleep_ms(duration_ms);

    outb(PC_SPEAKER_PORT, (uint8_t)(inb(PC_SPEAKER_PORT) & 0xFC));
}

void timer_format_uptime(char *buffer) {
    uint32_t total_seconds = timer_state.seconds;
    uint32_t days = total_seconds / 86400;
    uint32_t hours = (total_seconds % 86400) / 3600;
    uint32_t minutes = (total_seconds % 3600) / 60;
    uint32_t seconds = total_seconds % 60;

    int pos = 0;

    if (days > 0) {
        if (days >= 100) buffer[pos++] = '0' + (char)((days / 100) % 10);
        if (days >= 10)  buffer[pos++] = '0' + (char)((days / 10) % 10);
        buffer[pos++] = '0' + (char)(days % 10);
        buffer[pos++] = 'd';
        buffer[pos++] = ' ';
    }

    buffer[pos++] = '0' + (char)((hours / 10) % 10);
    buffer[pos++] = '0' + (char)(hours % 10);
    buffer[pos++] = ':';
    buffer[pos++] = '0' + (char)((minutes / 10) % 10);
    buffer[pos++] = '0' + (char)(minutes % 10);
    buffer[pos++] = ':';
    buffer[pos++] = '0' + (char)((seconds / 10) % 10);
    buffer[pos++] = '0' + (char)(seconds % 10);
    buffer[pos] = '\0';
}

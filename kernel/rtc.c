/**
 * Bengal Tiger OS - RTC (Real-Time Clock) Driver Implementation
 *
 * Reads date and time atomically from the Motorola MC146818 CMOS RTC.
 * Preserves interrupt flag state (never prematurely executes STI during boot)
 * and correctly interprets Status Register B bit 2 (DM: 0 = BCD, 1 = Binary).
 *
 * @file rtc.c
 * @author Bengal Tiger OS (BengalEmpire)
 * @version 1.0.0
 */

#include "rtc.h"
#include "common.h"

static int rtc_available = 0;

/**
 * Read a CMOS register while preserving caller's EFLAGS IF bit.
 */
static uint8_t cmos_read(uint8_t reg) {
    uint32_t flags = irq_save();
    outb(CMOS_ADDRESS, reg & 0x7F);
    inb(0x80);
    uint8_t val = inb(CMOS_DATA);
    irq_restore(flags);
    return val;
}

static uint8_t bcd_to_bin(uint8_t bcd) {
    return (uint8_t)((bcd & 0x0F) + ((bcd >> 4) * 10));
}

static void cmos_wait_uip(void) {
    int timeout = 100000;
    while (timeout-- > 0 && (cmos_read(CMOS_STATUS_A) & CMOS_STAT_A_UIP)) {
        inb(0x80);
    }
}

void rtc_init(void) {
    uint8_t status_b = cmos_read(CMOS_STATUS_B);
    if (status_b == 0xFF) {
        rtc_available = 0;
        return;
    }
    rtc_available = 1;
}

int rtc_read_time(rtc_time_t *time) {
    if (!rtc_available || !time) {
        return 0;
    }

    uint8_t s1, m1, h1, d1, mo1, y1, w1, c1;
    uint8_t s2, m2, h2, d2, mo2, y2, w2, c2;

    /* Standard double-read loop to avoid torn timestamps across second rollover */
    int attempts = 5;
    do {
        cmos_wait_uip();
        s1  = cmos_read(CMOS_SECONDS);
        m1  = cmos_read(CMOS_MINUTES);
        h1  = cmos_read(CMOS_HOURS);
        d1  = cmos_read(CMOS_DAY);
        mo1 = cmos_read(CMOS_MONTH);
        y1  = cmos_read(CMOS_YEAR);
        w1  = cmos_read(CMOS_WEEKDAY);
        c1  = cmos_read(CMOS_CENTURY);

        cmos_wait_uip();
        s2  = cmos_read(CMOS_SECONDS);
        m2  = cmos_read(CMOS_MINUTES);
        h2  = cmos_read(CMOS_HOURS);
        d2  = cmos_read(CMOS_DAY);
        mo2 = cmos_read(CMOS_MONTH);
        y2  = cmos_read(CMOS_YEAR);
        w2  = cmos_read(CMOS_WEEKDAY);
        c2  = cmos_read(CMOS_CENTURY);
    } while (--attempts > 0 &&
             (s1 != s2 || m1 != m2 || h1 != h2 || d1 != d2 ||
              mo1 != mo2 || y1 != y2 || w1 != w2 || c1 != c2));

    uint8_t status_b = cmos_read(CMOS_STATUS_B);
    /* MC146818 Status B bit 2 (0x04) is DM: 0 = BCD mode, 1 = Binary mode */
    int is_bcd  = (status_b & 0x04) ? 0 : 1;
    int is_24hr = (status_b & CMOS_STAT_B_24HR) ? 1 : 0;

    /* Extract 12-hour PM bit BEFORE BCD conversion */
    uint8_t pm = (!is_24hr && (h2 & 0x80)) ? 1 : 0;
    h2 &= 0x7F;

    if (is_bcd) {
        s2  = bcd_to_bin(s2);
        m2  = bcd_to_bin(m2);
        h2  = bcd_to_bin(h2);
        d2  = bcd_to_bin(d2);
        mo2 = bcd_to_bin(mo2);
        y2  = bcd_to_bin(y2);
        w2  = bcd_to_bin(w2);
        if (c2 != 0x00 && c2 != 0xFF) {
            c2 = bcd_to_bin(c2);
        } else {
            c2 = 0;
        }
    }

    if (!is_24hr) {
        if (pm) {
            h2 = (uint8_t)((h2 % 12) + 12);
        } else if (h2 == 12) {
            h2 = 0;
        }
    }

    time->seconds = (s2 < 60) ? s2 : 0;
    time->minutes = (m2 < 60) ? m2 : 0;
    time->hours   = (h2 < 24) ? h2 : 0;
    time->day     = (d2 >= 1 && d2 <= 31) ? d2 : 1;
    time->month   = (mo2 >= 1 && mo2 <= 12) ? mo2 : 1;
    time->weekday = w2;

    if (c2 >= 19 && c2 <= 21) {
        time->year = (uint16_t)c2 * 100 + y2;
    } else {
        time->year = (y2 >= 70) ? (uint16_t)(1900 + y2) : (uint16_t)(2000 + y2);
    }

    return 1;
}

void rtc_format_time(const rtc_time_t *time, char *buffer) {
    if (!time || !buffer) return;

    buffer[0] = '0' + (char)(time->hours / 10);
    buffer[1] = '0' + (char)(time->hours % 10);
    buffer[2] = ':';
    buffer[3] = '0' + (char)(time->minutes / 10);
    buffer[4] = '0' + (char)(time->minutes % 10);
    buffer[5] = ':';
    buffer[6] = '0' + (char)(time->seconds / 10);
    buffer[7] = '0' + (char)(time->seconds % 10);
    buffer[8] = '\0';
}

void rtc_format_date(const rtc_time_t *time, char *buffer) {
    if (!time || !buffer) return;

    buffer[0]  = '0' + (char)((time->year / 1000) % 10);
    buffer[1]  = '0' + (char)((time->year / 100) % 10);
    buffer[2]  = '0' + (char)((time->year / 10) % 10);
    buffer[3]  = '0' + (char)(time->year % 10);
    buffer[4]  = '-';
    buffer[5]  = '0' + (char)(time->month / 10);
    buffer[6]  = '0' + (char)(time->month % 10);
    buffer[7]  = '-';
    buffer[8]  = '0' + (char)(time->day / 10);
    buffer[9]  = '0' + (char)(time->day % 10);
    buffer[10] = '\0';
}

void rtc_format_datetime(const rtc_time_t *time, char *buffer) {
    if (!time || !buffer) return;

    rtc_format_date(time, buffer);
    buffer[10] = ' ';
    rtc_format_time(time, buffer + 11);
}

int rtc_is_present(void) {
    return rtc_available;
}

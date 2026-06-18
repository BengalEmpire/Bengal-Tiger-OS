/**
 * Bengal Tiger OS - RTC (Real-Time Clock) Driver Implementation
 *
 * @file rtc.c
 * @author Bengal Tiger OS (BengalEmpire)
 * @version 0.4.0
 */

#include "rtc.h"
#include "common.h"

/* Global RTC state */
static int rtc_available = 0;
static int rtc_bcd_mode = 1;   /* Assume BCD until we read status B */

/**
 * Read a CMOS register.
 * Must disable interrupts during access to prevent conflicts.
 */
static uint8_t cmos_read(uint8_t reg) {
    __asm__ volatile("cli");
    outb(CMOS_ADDRESS, reg);
    uint8_t val = inb(CMOS_DATA);
    __asm__ volatile("sti");
    return val;
}

/**
 * Write a CMOS register (careful - this can change settings).
 */
static void cmos_write(uint8_t reg, uint8_t val) {
    __asm__ volatile("cli");
    outb(CMOS_ADDRESS, reg);
    __attribute__((unused)) uint8_t dummy = inb(CMOS_DATA);
    outb(CMOS_DATA, val);
    __asm__ volatile("sti");
}

/**
 * Convert BCD to binary value.
 */
static uint8_t bcd_to_bin(uint8_t bcd) {
    return (bcd & 0x0F) + ((bcd >> 4) * 10);
}

/**
 * Wait for RTC update to complete (clear the Update-In-Progress flag).
 * The RTC is updating for about 2ms every second.
 */
static void cmos_wait_uip(void) {
    int timeout = 100000;
    while (timeout-- && (cmos_read(CMOS_STATUS_A) & CMOS_STAT_A_UIP)) {
        /* Small delay - I/O wait */
        inb(0x80);
    }
}

void rtc_init(void) {
    /* Check if RTC responds */
    uint8_t status_b = cmos_read(CMOS_STATUS_B);

    /* If we get 0xFF or 0x00, RTC may not be responding */
    if (status_b == 0xFF) {
        rtc_available = 0;
        return;
    }

    rtc_available = 1;

    /* Determine if RTC is in BCD or binary mode */
    rtc_bcd_mode = (status_b & CMOS_STAT_B_BCD) ? 1 : 0;

    /* Enable 24-hour mode if not already */
    if (!(status_b & CMOS_STAT_B_24HR)) {
        cmos_write(CMOS_STATUS_B, status_b | CMOS_STAT_B_24HR);
    }
}

int rtc_read_time(rtc_time_t *time) {
    if (!rtc_available || !time) {
        return 0;
    }

    uint8_t seconds, minutes, hours, day, month, year, weekday, century;

    /* Read all registers atomically (wait for UIP to clear) */
    cmos_wait_uip();

    seconds = cmos_read(CMOS_SECONDS);
    minutes = cmos_read(CMOS_MINUTES);
    hours   = cmos_read(CMOS_HOURS);
    day     = cmos_read(CMOS_DAY);
    month   = cmos_read(CMOS_MONTH);
    year    = cmos_read(CMOS_YEAR);
    weekday = cmos_read(CMOS_WEEKDAY);
    century = cmos_read(CMOS_CENTURY);

    /* Convert from BCD if needed */
    if (rtc_bcd_mode) {
        seconds = bcd_to_bin(seconds);
        minutes = bcd_to_bin(minutes);
        hours   = bcd_to_bin(hours);
        day     = bcd_to_bin(day);
        month   = bcd_to_bin(month);
        year    = bcd_to_bin(year);
        weekday = bcd_to_bin(weekday);
        century = bcd_to_bin(century);
    }

    /* Handle 12-hour format (bit 7 set = PM) */
    if (!(cmos_read(CMOS_STATUS_B) & CMOS_STAT_B_24HR)) {
        uint8_t pm = hours & 0x80;
        hours &= 0x7F;
        if (pm) {
            hours = (hours % 12) + 12;
        } else if (hours == 12) {
            hours = 0;
        }
    }

    /* Fill time structure */
    time->seconds = seconds;
    time->minutes = minutes;
    time->hours   = hours;
    time->day     = day;
    time->month   = month;
    time->weekday = weekday;

    /* Calculate full year */
    if (century >= 19) {
        time->year = (uint16_t)century * 100 + year;
    } else {
        /* BIOS didn't provide century byte */
        if (year >= 70) {
            time->year = 1900 + year;
        } else {
            time->year = 2000 + year;
        }
    }

    return 1;
}

void rtc_format_time(const rtc_time_t *time, char *buffer) {
    if (!time || !buffer) return;

    buffer[0] = '0' + (time->hours / 10);
    buffer[1] = '0' + (time->hours % 10);
    buffer[2] = ':';
    buffer[3] = '0' + (time->minutes / 10);
    buffer[4] = '0' + (time->minutes % 10);
    buffer[5] = ':';
    buffer[6] = '0' + (time->seconds / 10);
    buffer[7] = '0' + (time->seconds % 10);
    buffer[8] = '\0';
}

void rtc_format_date(const rtc_time_t *time, char *buffer) {
    if (!time || !buffer) return;

    buffer[0]  = '0' + ((time->year / 1000) % 10);
    buffer[1]  = '0' + ((time->year / 100) % 10);
    buffer[2]  = '0' + ((time->year / 10) % 10);
    buffer[3]  = '0' + (time->year % 10);
    buffer[4]  = '-';
    buffer[5]  = '0' + (time->month / 10);
    buffer[6]  = '0' + (time->month % 10);
    buffer[7]  = '-';
    buffer[8]  = '0' + (time->day / 10);
    buffer[9]  = '0' + (time->day % 10);
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

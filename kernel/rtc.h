/**
 * Bengal Tiger OS - RTC (Real-Time Clock) Driver
 *
 * Reads date and time from the CMOS RTC (Motorola MC146818).
 * Provides functions to get current time, date, and formatted strings.
 *
 * @file rtc.h
 * @author Bengal Tiger OS (BengalEmpire)
 * @version 0.4.0
 */

#ifndef RTC_H
#define RTC_H

#include "common.h"

/* CMOS I/O Ports */
#define CMOS_ADDRESS     0x70
#define CMOS_DATA        0x71

/* CMOS Register Addresses */
#define CMOS_SECONDS     0x00
#define CMOS_MINUTES     0x02
#define CMOS_HOURS       0x04
#define CMOS_WEEKDAY     0x06
#define CMOS_DAY         0x07
#define CMOS_MONTH       0x08
#define CMOS_YEAR        0x09
#define CMOS_STATUS_A    0x0A
#define CMOS_STATUS_B    0x0B
#define CMOS_STATUS_C    0x0C
#define CMOS_STATUS_D    0x0D
#define CMOS_CENTURY     0x32   /* ACPI century register */

/* Status Register A Bits */
#define CMOS_STAT_A_UIP  0x80   /* Update in progress */

/* Status Register B Bits */
#define CMOS_STAT_B_24HR 0x02   /* 24-hour format */
#define CMOS_STAT_B_BCD  0x04   /* BCD mode */
#define CMOS_STAT_B_PIE  0x40   /* Periodic interrupt enable */

/**
 * Time structure with all components.
 */
typedef struct {
    uint8_t seconds;     /* 0-59 */
    uint8_t minutes;     /* 0-59 */
    uint8_t hours;       /* 0-23 */
    uint8_t day;         /* 1-31 */
    uint8_t month;       /* 1-12 */
    uint16_t year;       /* Full year (e.g., 2026) */
    uint8_t weekday;     /* 1=Sunday, 7=Saturday */
} rtc_time_t;

/**
 * Initialize RTC driver.
 * Detects RTC presence and configures reasonable defaults.
 */
void rtc_init(void);

/**
 * Read the current time from CMOS RTC.
 * Handles update-in-progress (UIP) flag and BCD/binary conversion.
 *
 * @param time Pointer to rtc_time_t structure to fill
 * @return 1 on success, 0 on failure
 */
int rtc_read_time(rtc_time_t *time);

/**
 * Format time as "HH:MM:SS" string.
 * @param time Pointer to time structure
 * @param buffer Output buffer (at least 9 bytes)
 */
void rtc_format_time(const rtc_time_t *time, char *buffer);

/**
 * Format date as "YYYY-MM-DD" string.
 * @param time Pointer to time structure
 * @param buffer Output buffer (at least 11 bytes)
 */
void rtc_format_date(const rtc_time_t *time, char *buffer);

/**
 * Format full date-time as "YYYY-MM-DD HH:MM:SS".
 * @param time Pointer to time structure
 * @param buffer Output buffer (at least 21 bytes)
 */
void rtc_format_datetime(const rtc_time_t *time, char *buffer);

/**
 * Check if RTC is available (non-destructive test).
 * @return 1 if RTC responds, 0 if not.
 */
int rtc_is_present(void);

#endif /* RTC_H */

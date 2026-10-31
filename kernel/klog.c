/**
 * Bengal Tiger OS - Kernel Diagnostic Log Implementation
 *
 * @file klog.c
 * @author Bengal Tiger OS (BengalEmpire)
 * @version 1.0.0
 */

#include "klog.h"
#include "serial.h"
#include "timer.h"

static char klog_buf[KLOG_BUFFER_SIZE];
static uint32_t klog_len = 0;

static void klog_append_char(char c) {
    if (klog_len < KLOG_BUFFER_SIZE - 1) {
        klog_buf[klog_len++] = c;
        klog_buf[klog_len] = '\0';
    } else {
        /* Drop oldest line to make room */
        uint32_t drop = 256;
        for (uint32_t i = 0; i < KLOG_BUFFER_SIZE / 2; i++) {
            if (klog_buf[i] == '\n') {
                drop = i + 1;
                break;
            }
        }
        uint32_t remain = klog_len - drop;
        memmove(klog_buf, klog_buf + drop, remain);
        klog_len = remain;
        klog_buf[klog_len++] = c;
        klog_buf[klog_len] = '\0';
    }
}

void klog_init(void) {
    uint32_t flags = irq_save();
    memset(klog_buf, 0, sizeof(klog_buf));
    klog_len = 0;
    irq_restore(flags);
}

void klog_write(const char *msg) {
    if (!msg) return;
    uint32_t flags = irq_save();
    while (*msg) {
        klog_append_char(*msg++);
    }
    irq_restore(flags);
}

void klog_write_hex(uint32_t val) {
    char hex[12] = "0x00000000";
    const char *digits = "0123456789ABCDEF";
    for (int i = 9; i >= 2; i--) {
        hex[i] = digits[val & 0x0F];
        val >>= 4;
    }
    klog_write(hex);
}

void klog_write_int(int32_t val) {
    char buf[16];
    k_itoa(val, buf, 10);
    klog_write(buf);
}

void klog_writeln(const char *subsys, const char *msg) {
    uint32_t ms = timer_get_ms();
    uint32_t sec = ms / 1000;
    uint32_t rem = (ms % 1000) / 10;

    char prefix[32];
    int p = 0;
    prefix[p++] = '[';
    char sbuf[12];
    k_utoa(sec, sbuf, 10);
    int sl = strlen(sbuf);
    for (int i = sl; i < 4; i++) prefix[p++] = ' ';
    for (int i = 0; i < sl; i++) prefix[p++] = sbuf[i];
    prefix[p++] = '.';
    prefix[p++] = '0' + (char)((rem / 10) % 10);
    prefix[p++] = '0' + (char)(rem % 10);
    prefix[p++] = ']';
    prefix[p++] = ' ';
    prefix[p] = '\0';

    klog_write(prefix);
    serial_write_str(prefix);

    if (subsys && subsys[0]) {
        klog_write(subsys);
        klog_write(": ");
        serial_write_str(subsys);
        serial_write_str(": ");
    }
    if (msg) {
        klog_write(msg);
        serial_write_str(msg);
    }
    klog_write("\n");
    serial_write_str("\n");
}

const char* klog_get_buffer(uint32_t *out_len) {
    if (out_len) *out_len = klog_len;
    return klog_buf;
}

void klog_clear(void) {
    klog_init();
}

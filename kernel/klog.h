/**
 * Bengal Tiger OS - Kernel Diagnostic Log Ring Buffer (dmesg)
 *
 * Records kernel boot messages and subsystem events into an in-memory
 * circular buffer while mirroring them to COM1 serial output.
 *
 * @file klog.h
 * @author Bengal Tiger OS (BengalEmpire)
 * @version 1.0.0
 */

#ifndef KLOG_H
#define KLOG_H

#include "common.h"

#define KLOG_BUFFER_SIZE 8192

void klog_init(void);
void klog_write(const char *msg);
void klog_writeln(const char *subsys, const char *msg);
void klog_write_hex(uint32_t val);
void klog_write_int(int32_t val);
const char* klog_get_buffer(uint32_t *out_len);
void klog_clear(void);

#endif /* KLOG_H */

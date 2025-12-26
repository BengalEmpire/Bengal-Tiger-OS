/**
 * Bengal Tiger OS - Kernel Panic Handler
 * 
 * Provides a safe halt mechanism when unrecoverable errors occur.
 * Displays diagnostic information and halts the system.
 * 
 * @file panic.h
 * @author Bengal Tiger OS (BengalEmpire)
 * @version 0.3.0
 */

#ifndef PANIC_H
#define PANIC_H

#include "common.h"

/* Exception Names */
extern const char *exception_names[32];

void kernel_panic(const char *message);

void kernel_panic_at(const char *message, const char *file, int line);

/**
 * Handle a CPU exception.
 * @param r Pointer to saved CPU registers
 */
void exception_handler(struct regs *r);

/**
 * Assert macro for debug builds.
 */
#define ASSERT(condition) \
    do { \
        if (!(condition)) { \
            kernel_panic_at("Assertion failed: " #condition, __FILE__, __LINE__); \
        } \
    } while (0)

/**
 * Panic macro with automatic file/line info.
 */
#define PANIC(msg) kernel_panic_at(msg, __FILE__, __LINE__)

#endif /* PANIC_H */

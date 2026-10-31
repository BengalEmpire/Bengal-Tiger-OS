/**
 * Bengal Tiger OS - Common Definitions and Utilities
 *
 * Provides standard fixed-width integer types, I/O port instructions,
 * interrupt flag save/restore helpers, endianness conversions,
 * memory routines, and string/formatting utilities.
 *
 * @file common.h
 * @author Bengal Tiger OS (BengalEmpire)
 * @version 1.0.0
 */

#ifndef COMMON_H
#define COMMON_H

#ifdef HOST_TEST
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <string.h>
#else
typedef unsigned char       uint8_t;
typedef signed char         int8_t;
typedef unsigned short      uint16_t;
typedef signed short        int16_t;
typedef unsigned int        uint32_t;
typedef signed int          int32_t;
typedef unsigned long long  uint64_t;
typedef signed long long    int64_t;
typedef unsigned int        size_t;
typedef signed int          ssize_t;
typedef unsigned int        uintptr_t;

typedef int bool;
#define true  1
#define false 0

#define NULL ((void*)0)
#endif

/* ============================================ */
/* I/O Port & CPU Control Instructions          */
/* ============================================ */

#ifdef __i386__
void outb(uint16_t port, uint8_t val);
void outw(uint16_t port, uint16_t val);
uint8_t inb(uint16_t port);
uint16_t inw(uint16_t port);

static inline void outl(uint16_t port, uint32_t val) {
    __asm__ volatile("outl %0, %1" : : "a"(val), "Nd"(port));
}

static inline uint32_t inl(uint16_t port) {
    uint32_t ret;
    __asm__ volatile("inl %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

static inline void cli(void) {
    __asm__ volatile("cli" ::: "memory");
}

static inline void sti(void) {
    __asm__ volatile("sti" ::: "memory");
}

static inline void hlt(void) {
    __asm__ volatile("hlt");
}

static inline uint32_t irq_save(void) {
    uint32_t flags;
    __asm__ volatile("pushfl\n\tpopl %0\n\tcli" : "=r"(flags) : : "memory");
    return flags;
}

static inline void irq_restore(uint32_t flags) {
    __asm__ volatile("pushl %0\n\tpopfl" : : "r"(flags) : "memory", "cc");
}

static inline int irq_are_enabled(void) {
    uint32_t flags;
    __asm__ volatile("pushfl\n\tpopl %0" : "=r"(flags));
    return (flags & (1 << 9)) ? 1 : 0;
}
#else
/* Host-mode stubs for unit testing */
static inline void outb(uint16_t port, uint8_t val) { (void)port; (void)val; }
static inline void outw(uint16_t port, uint16_t val) { (void)port; (void)val; }
static inline uint8_t inb(uint16_t port) { (void)port; return 0; }
static inline uint16_t inw(uint16_t port) { (void)port; return 0; }
static inline void outl(uint16_t port, uint32_t val) { (void)port; (void)val; }
static inline uint32_t inl(uint16_t port) { (void)port; return 0; }
static inline void cli(void) {}
static inline void sti(void) {}
static inline void hlt(void) {}
static inline uint32_t irq_save(void) { return 0; }
static inline void irq_restore(uint32_t flags) { (void)flags; }
static inline int irq_are_enabled(void) { return 0; }
#endif

/* ============================================ */
/* Byte-Order Helpers (Network Big-Endian)      */
/* ============================================ */

static inline uint16_t bswap16(uint16_t v) {
    return (uint16_t)((v >> 8) | (v << 8));
}

static inline uint32_t bswap32(uint32_t v) {
    return ((v & 0x000000FFU) << 24) |
           ((v & 0x0000FF00U) << 8)  |
           ((v & 0x00FF0000U) >> 8)  |
           ((v & 0xFF000000U) >> 24);
}

#ifndef htons
#define htons(x) bswap16(x)
#define ntohs(x) bswap16(x)
#define htonl(x) bswap32(x)
#define ntohl(x) bswap32(x)
#endif

/* ============================================ */
/* Memory & String Functions                    */
/* ============================================ */

#ifndef HOST_TEST
void memset(void *dest, uint8_t val, uint32_t len);
void memcpy(void *dest, const void *src, uint32_t len);
void *memmove(void *dest, const void *src, uint32_t len);
int memcmp(const void *s1, const void *s2, uint32_t n);

int strlen(const char *s);
int strcmp(const char *s1, const char *s2);
int strncmp(const char *s1, const char *s2, uint32_t n);
int strcasecmp(const char *s1, const char *s2);
int strncasecmp(const char *s1, const char *s2, uint32_t n);
char *strcpy(char *dest, const char *src);
char *strncpy(char *dest, const char *src, uint32_t n);
char *strcat(char *dest, const char *src);
char *strchr(const char *s, int c);
char *strstr(const char *haystack, const char *needle);
#else
#include <strings.h>
#endif

int k_atoi(const char *s);
void k_itoa(int32_t val, char *buf, int base);
void k_utoa(uint32_t val, char *buf, int base);

/* ============================================ */
/* CPU Register State (for interrupts)          */
/* ============================================ */

struct regs {
    uint32_t gs, fs, es, ds;                         /* Segment selectors */
    uint32_t edi, esi, ebp, esp, ebx, edx, ecx, eax; /* pusha registers */
    uint32_t int_no, err_code;                       /* Interrupt number and error */
    uint32_t eip, cs, eflags, useresp, ss;           /* Pushed by CPU */
};

/* ============================================ */
/* Utility Macros                               */
/* ============================================ */

#define MIN(a, b)        ((a) < (b) ? (a) : (b))
#define MAX(a, b)        ((a) > (b) ? (a) : (b))
#define ARRAY_SIZE(arr)  (sizeof(arr) / sizeof((arr)[0]))

#define ALIGN_UP(val, align)   (((val) + (align) - 1) & ~((align) - 1))
#define ALIGN_DOWN(val, align) ((val) & ~((align) - 1))
#define IS_POWER_OF_2(x)       ((x) != 0 && ((x) & ((x) - 1)) == 0)

#define UNUSED(x) (void)(x)
#define MEMORY_BARRIER() __asm__ volatile("" ::: "memory")

#endif /* COMMON_H */

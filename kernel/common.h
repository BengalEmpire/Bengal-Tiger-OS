/**
 * Bengal Tiger OS - Common Definitions and Utilities
 * 
 * This header provides:
 * - Standard integer types
 * - NULL pointer definition
 * - I/O port access functions
 * - Memory manipulation functions
 * - String functions
 * - CPU register structure
 * 
 * @file common.h
 * @author Bengal Tiger OS (BengalEmpire)
 * @version 0.3.0
 */

#ifndef COMMON_H
#define COMMON_H

/* ============================================ */
/* Standard Integer Types                       */
/* ============================================ */

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

/* Boolean type */
typedef int bool;
#define true  1
#define false 0

/* NULL pointer */
#define NULL ((void*)0)

/* ============================================ */
/* I/O Port Access Functions                    */
/* ============================================ */

/**
 * Write a byte to an I/O port.
 * @param port The port number (0-65535)
 * @param val The byte value to write
 */
void outb(uint16_t port, uint8_t val);

/**
 * Write a word (16-bit) to an I/O port.
 * @param port The port number
 * @param val The word value to write
 */
void outw(uint16_t port, uint16_t val);

/**
 * Read a byte from an I/O port.
 * @param port The port number
 * @return The byte value read
 */
uint8_t inb(uint16_t port);

/**
 * Read a word (16-bit) from an I/O port.
 * @param port The port number
 * @return The word value read
 */
uint16_t inw(uint16_t port);

/* ============================================ */
/* Memory Functions                             */
/* ============================================ */

/**
 * Set a region of memory to a specific byte value.
 * @param dest Destination pointer
 * @param val Byte value to set
 * @param len Number of bytes to set
 */
void memset(void *dest, uint8_t val, uint32_t len);

/**
 * Copy a region of memory.
 * @param dest Destination pointer
 * @param src Source pointer
 * @param len Number of bytes to copy
 */
void memcpy(void *dest, const void *src, uint32_t len);

/**
 * Compare two memory regions.
 * @param s1 First memory region
 * @param s2 Second memory region
 * @param n Number of bytes to compare
 * @return 0 if equal, <0 if s1<s2, >0 if s1>s2
 */
int memcmp(const void *s1, const void *s2, uint32_t n);

/* ============================================ */
/* String Functions                             */
/* ============================================ */

/**
 * Get the length of a null-terminated string.
 * @param s The string
 * @return Length in bytes (excluding null terminator)
 */
int strlen(const char *s);

/**
 * Compare two strings.
 * @param s1 First string
 * @param s2 Second string
 * @return 0 if equal, <0 if s1<s2, >0 if s1>s2
 */
int strcmp(const char *s1, const char *s2);

/**
 * Compare two strings up to n characters.
 * @param s1 First string
 * @param s2 Second string
 * @param n Maximum characters to compare
 * @return 0 if equal, <0 if s1<s2, >0 if s1>s2
 */
int strncmp(const char *s1, const char *s2, uint32_t n);

/**
 * Copy a string.
 * @param dest Destination buffer
 * @param src Source string
 * @return Pointer to dest
 */
char *strcpy(char *dest, const char *src);

/**
 * Copy a string with size limit.
 * @param dest Destination buffer
 * @param src Source string
 * @param n Maximum characters to copy
 * @return Pointer to dest
 */
char *strncpy(char *dest, const char *src, uint32_t n);

/**
 * Concatenate strings.
 * @param dest Destination buffer (must have space)
 * @param src Source string to append
 * @return Pointer to dest
 */
char *strcat(char *dest, const char *src);

/**
 * Find character in string.
 * @param s The string to search
 * @param c The character to find
 * @return Pointer to first occurrence, or NULL
 */
char *strchr(const char *s, int c);

/* ============================================ */
/* CPU Register State (for interrupts)          */
/* ============================================ */

/**
 * Structure holding saved CPU register state.
 * Pushed by ISR stubs in isr.s
 */
struct regs {
    uint32_t gs, fs, es, ds;                     /* Segment selectors */
    uint32_t edi, esi, ebp, esp, ebx, edx, ecx, eax;  /* pusha registers */
    uint32_t int_no, err_code;                   /* Interrupt number and error */
    uint32_t eip, cs, eflags, useresp, ss;       /* Pushed by CPU */
};

/* ============================================ */
/* Utility Macros                               */
/* ============================================ */

/* Get minimum/maximum of two values */
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define MAX(a, b) ((a) > (b) ? (a) : (b))

/* Align value up to boundary (must be power of 2) */
#define ALIGN_UP(val, align) (((val) + (align) - 1) & ~((align) - 1))

/* Align value down to boundary */
#define ALIGN_DOWN(val, align) ((val) & ~((align) - 1))

/* Check if value is power of 2 */
#define IS_POWER_OF_2(x) ((x) != 0 && ((x) & ((x) - 1)) == 0)

/* Unused parameter marker */
#define UNUSED(x) (void)(x)

/* Compiler memory barrier */
#define MEMORY_BARRIER() __asm__ volatile("" ::: "memory")

#endif /* COMMON_H */
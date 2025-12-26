/**
 * Bengal Tiger OS - Common Functions Implementation
 * 
 * @file common.c
 * @author Bengal Tiger OS (BengalEmpire)
 * @version 0.3.0
 */

#include "common.h"

/* ============================================ */
/* I/O Port Functions                           */
/* ============================================ */

void outb(uint16_t port, uint8_t val) {
    __asm__ volatile("outb %0, %1" : : "a"(val), "Nd"(port));
}

void outw(uint16_t port, uint16_t val) {
    __asm__ volatile("outw %0, %1" : : "a"(val), "Nd"(port));
}

uint8_t inb(uint16_t port) {
    uint8_t ret;
    __asm__ volatile("inb %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

uint16_t inw(uint16_t port) {
    uint16_t ret;
    __asm__ volatile("inw %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

/* ============================================ */
/* Memory Functions                             */
/* ============================================ */

void memset(void *dest, uint8_t val, uint32_t len) {
    uint8_t *d = (uint8_t*)dest;
    
    /* Optimize for long runs using 32-bit writes */
    if (len >= 4) {
        /* Create 32-bit value with val repeated 4 times */
        uint32_t val32 = val | (val << 8) | (val << 16) | (val << 24);
        
        /* Align to 4 bytes first */
        while (((uint32_t)d & 3) && len > 0) {
            *d++ = val;
            len--;
        }
        
        /* Write 32 bits at a time */
        uint32_t *d32 = (uint32_t*)d;
        while (len >= 4) {
            *d32++ = val32;
            len -= 4;
        }
        
        d = (uint8_t*)d32;
    }
    
    /* Handle remaining bytes */
    while (len > 0) {
        *d++ = val;
        len--;
    }
}

void memcpy(void *dest, const void *src, uint32_t len) {
    uint8_t *d = (uint8_t*)dest;
    const uint8_t *s = (const uint8_t*)src;
    
    /* Check for overlap - copy backwards if needed */
    if (d > s && d < s + len) {
        d += len;
        s += len;
        while (len--) {
            *--d = *--s;
        }
    } else {
        /* Optimize with 32-bit copies when aligned */
        if (len >= 4 && ((uint32_t)d & 3) == 0 && ((uint32_t)s & 3) == 0) {
            uint32_t *d32 = (uint32_t*)d;
            const uint32_t *s32 = (const uint32_t*)s;
            
            while (len >= 4) {
                *d32++ = *s32++;
                len -= 4;
            }
            
            d = (uint8_t*)d32;
            s = (const uint8_t*)s32;
        }
        
        while (len--) {
            *d++ = *s++;
        }
    }
}

int memcmp(const void *s1, const void *s2, uint32_t n) {
    const uint8_t *p1 = (const uint8_t*)s1;
    const uint8_t *p2 = (const uint8_t*)s2;
    
    while (n--) {
        if (*p1 != *p2) {
            return *p1 - *p2;
        }
        p1++;
        p2++;
    }
    
    return 0;
}

/* ============================================ */
/* String Functions                             */
/* ============================================ */

int strlen(const char *s) {
    int len = 0;
    while (s[len]) len++;
    return len;
}

int strcmp(const char *s1, const char *s2) {
    while (*s1 && (*s1 == *s2)) {
        s1++;
        s2++;
    }
    return *(const uint8_t*)s1 - *(const uint8_t*)s2;
}

int strncmp(const char *s1, const char *s2, uint32_t n) {
    while (n && *s1 && (*s1 == *s2)) {
        s1++;
        s2++;
        n--;
    }
    if (n == 0) return 0;
    return *(const uint8_t*)s1 - *(const uint8_t*)s2;
}

char *strcpy(char *dest, const char *src) {
    char *d = dest;
    while ((*d++ = *src++));
    return dest;
}

char *strncpy(char *dest, const char *src, uint32_t n) {
    char *d = dest;
    
    while (n && (*d = *src)) {
        d++;
        src++;
        n--;
    }
    
    /* Pad with nulls */
    while (n--) {
        *d++ = '\0';
    }
    
    return dest;
}

char *strcat(char *dest, const char *src) {
    char *d = dest;
    
    /* Find end of dest */
    while (*d) d++;
    
    /* Copy src */
    while ((*d++ = *src++));
    
    return dest;
}

char *strchr(const char *s, int c) {
    while (*s) {
        if (*s == (char)c) {
            return (char*)s;
        }
        s++;
    }
    
    /* Check for null terminator */
    if (c == '\0') {
        return (char*)s;
    }
    
    return NULL;
}
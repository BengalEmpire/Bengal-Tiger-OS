/**
 * Bengal Tiger OS - Common Functions Implementation
 *
 * @file common.c
 * @author Bengal Tiger OS (BengalEmpire)
 * @version 1.0.0
 */

#include "common.h"

#ifdef __i386__
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
#endif

#ifndef HOST_TEST
void memset(void *dest, uint8_t val, uint32_t len) {
    uint8_t *d = (uint8_t*)dest;

    if (len >= 4) {
        uint32_t val32 = (uint32_t)val | ((uint32_t)val << 8) |
                         ((uint32_t)val << 16) | ((uint32_t)val << 24);

        while (((uintptr_t)d & 3) && len > 0) {
            *d++ = val;
            len--;
        }

        uint32_t *d32 = (uint32_t*)d;
        while (len >= 4) {
            *d32++ = val32;
            len -= 4;
        }

        d = (uint8_t*)d32;
    }

    while (len > 0) {
        *d++ = val;
        len--;
    }
}

void memcpy(void *dest, const void *src, uint32_t len) {
    uint8_t *d = (uint8_t*)dest;
    const uint8_t *s = (const uint8_t*)src;

    if (d == s || len == 0) return;

    if (d > s && d < s + len) {
        d += len;
        s += len;
        while (len--) {
            *--d = *--s;
        }
    } else {
        if (len >= 4 && (((uintptr_t)d & 3) == 0) && (((uintptr_t)s & 3) == 0)) {
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

void *memmove(void *dest, const void *src, uint32_t len) {
    memcpy(dest, src, len);
    return dest;
}

int memcmp(const void *s1, const void *s2, uint32_t n) {
    const uint8_t *p1 = (const uint8_t*)s1;
    const uint8_t *p2 = (const uint8_t*)s2;

    while (n--) {
        if (*p1 != *p2) {
            return (int)*p1 - (int)*p2;
        }
        p1++;
        p2++;
    }

    return 0;
}

static inline char to_lower_ch(char c) {
    return (c >= 'A' && c <= 'Z') ? (char)(c + ('a' - 'A')) : c;
}

int strlen(const char *s) {
    if (!s) return 0;
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

int strcasecmp(const char *s1, const char *s2) {
    while (*s1 && (to_lower_ch(*s1) == to_lower_ch(*s2))) {
        s1++;
        s2++;
    }
    return (uint8_t)to_lower_ch(*s1) - (uint8_t)to_lower_ch(*s2);
}

int strncasecmp(const char *s1, const char *s2, uint32_t n) {
    while (n && *s1 && (to_lower_ch(*s1) == to_lower_ch(*s2))) {
        s1++;
        s2++;
        n--;
    }
    if (n == 0) return 0;
    return (uint8_t)to_lower_ch(*s1) - (uint8_t)to_lower_ch(*s2);
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

    while (n--) {
        *d++ = '\0';
    }

    return dest;
}

char *strcat(char *dest, const char *src) {
    char *d = dest;
    while (*d) d++;
    while ((*d++ = *src++));
    return dest;
}

char *strchr(const char *s, int c) {
    while (*s) {
        if (*s == (char)c) return (char*)s;
        s++;
    }
    if (c == '\0') return (char*)s;
    return NULL;
}

char *strstr(const char *haystack, const char *needle) {
    if (!*needle) return (char*)haystack;
    uint32_t nlen = (uint32_t)strlen(needle);
    while (*haystack) {
        if (*haystack == *needle && strncmp(haystack, needle, nlen) == 0) {
            return (char*)haystack;
        }
        haystack++;
    }
    return NULL;
}
#endif /* !HOST_TEST */

int k_atoi(const char *s) {
    if (!s) return 0;
    while (*s == ' ' || *s == '\t') s++;
    int sign = 1;
    if (*s == '-') {
        sign = -1;
        s++;
    } else if (*s == '+') {
        s++;
    }
    int val = 0;
    while (*s >= '0' && s[0] <= '9') {
        val = val * 10 + (*s - '0');
        s++;
    }
    return val * sign;
}

void k_utoa(uint32_t val, char *buf, int base) {
    const char *digits = "0123456789ABCDEF";
    char tmp[36];
    int pos = 0;

    if (base < 2 || base > 16) base = 10;

    if (val == 0) {
        buf[0] = '0';
        buf[1] = '\0';
        return;
    }

    while (val > 0) {
        tmp[pos++] = digits[val % (uint32_t)base];
        val /= (uint32_t)base;
    }

    int out = 0;
    while (pos > 0) {
        buf[out++] = tmp[--pos];
    }
    buf[out] = '\0';
}

void k_itoa(int32_t val, char *buf, int base) {
    if (val < 0 && base == 10) {
        *buf++ = '-';
        k_utoa((uint32_t)(-(int64_t)val), buf, base);
    } else {
        k_utoa((uint32_t)val, buf, base);
    }
}

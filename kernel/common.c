// kernel/common.c

#include "common.h"

void outb(uint16_t port, uint8_t val) {
    __asm__ volatile("outb %0, %1" : : "a"(val), "Nd"(port));
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

void memset(void *dest, uint8_t val, uint32_t len) {
    uint8_t *d = (uint8_t*)dest;
    for (uint32_t i = 0; i < len; i++) d[i] = val;
}
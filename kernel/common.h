#ifndef COMMON_H
#define COMMON_H

typedef unsigned int   uint32_t;
typedef unsigned short uint16_t;
typedef unsigned char  uint8_t;

void outb(uint16_t port, uint8_t val);
uint8_t inb(uint16_t port);
uint16_t inw(uint16_t port);
void memset(void *dest, uint8_t val, uint32_t len);

#endif
#ifndef DISK_H
#define DISK_H

#include "common.h"

void ata_read_sector(uint32_t lba, uint8_t *buf);

#endif
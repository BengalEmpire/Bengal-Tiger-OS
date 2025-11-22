#ifndef SCHEDULER_H
#define SCHEDULER_H

#include "common.h"

struct regs;

extern volatile uint32_t tick;

void scheduler_init();
void scheduler_tick(struct regs *r);

#endif
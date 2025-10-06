#ifndef SCHEDULER_H
#define SCHEDULER_H

#include "common.h"  // For uint32_t etc.

struct regs;  // Forward declare

void scheduler_init();
void scheduler_tick(struct regs *r);

#endif
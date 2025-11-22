#include "scheduler.h"
#include "common.h"
#include "paging.h"

volatile uint32_t tick = 0;

struct task {
    struct regs regs;
    uint32_t page_dir;
    struct task *next;
};

static struct task *current_task;

void scheduler_init() {
    current_task = (struct task*)pmm_alloc_frame();  // Now valid because paging.h is included
    memset(current_task, 0, sizeof(struct task));
    current_task->next = current_task;
}

void scheduler_tick(struct regs *r) {
    (void)r; // Silence the "unused parameter" warning
    tick++;
    // Stub: Increment counter on VGA (moved to main for demo)
}
// kernel/scheduler.c (stub)

#include "scheduler.h"
#include "common.h"

struct task {
    struct regs regs;
    uint32_t page_dir;
    struct task *next;
};

static struct task *current_task;

void scheduler_init() {
    current_task = (struct task*)pmm_alloc_frame();  // Alloc stub
    memset(current_task, 0, sizeof(struct task));
    current_task->next = current_task;
}

void scheduler_tick(struct regs *r) {
    // Stub: Increment counter on VGA
    static int counter = 0;
    volatile uint16_t *video = (volatile uint16_t*)0xB8000;
    int row3_offset = 160;  // Row 3
    char buf[20] = "Count: ";
    int n = 7;
    int tmp = counter++;
    if (tmp == 0) buf[n++] = '0';
    while (tmp > 0) {
        buf[n++] = '0' + (tmp % 10);
        tmp /= 10;
    }
    for (int k = 0; k < n; k++) {
        video[row3_offset + k] = buf[k] | (0x07 << 8);
    }
}
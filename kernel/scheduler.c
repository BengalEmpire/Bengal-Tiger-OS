/**
 * Bengal Tiger OS - Scheduler Implementation (Stub)
 * 
 * @file scheduler.c
 * @author Bengal Tiger OS (BengalEmpire)
 * @version 0.3.0
 */

#include "scheduler.h"
#include "common.h"
#include "paging.h"
#include "heap.h"

/* Legacy tick counter (maintained for compatibility) */
volatile uint32_t tick = 0;

/* Current task (single-task for now) */
static task_t kernel_task;
static task_t *current_task = NULL;

void scheduler_init(void) {
    /* Initialize the kernel task */
    memset(&kernel_task, 0, sizeof(task_t));
    
    kernel_task.id = 0;
    strcpy(kernel_task.name, "kernel");
    kernel_task.state = TASK_STATE_RUNNING;
    kernel_task.next = &kernel_task;
    kernel_task.prev = &kernel_task;
    
    current_task = &kernel_task;
}

void scheduler_tick(struct regs *r) {
    UNUSED(r);
    
    /* Increment legacy tick counter */
    tick++;
    
    /* 
     * Future: This is where task switching would happen
     * 
     * if (should_switch()) {
     *     save_context(current_task, r);
     *     current_task = pick_next_task();
     *     restore_context(current_task, r);
     * }
     */
}

task_t* scheduler_get_current(void) {
    return current_task;
}

void scheduler_yield(void) {
    /* 
     * Future: Force a context switch
     * For now, just HLT until next interrupt
     */
    __asm__ volatile("hlt");
}

int scheduler_add_task(task_t *task) {
    /* Stub: Would add task to ready queue */
    UNUSED(task);
    return -1;  /* Not implemented */
}

void scheduler_remove_task(task_t *task) {
    /* Stub: Would remove task from scheduler */
    UNUSED(task);
}

void scheduler_block(void) {
    /* Stub: Would block current task */
}

void scheduler_unblock(task_t *task) {
    /* Stub: Would move task from blocked to ready */
    UNUSED(task);
}
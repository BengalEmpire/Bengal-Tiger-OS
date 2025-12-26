/**
 * Bengal Tiger OS - Scheduler (Stub)
 * 
 * Task scheduling infrastructure stub.
 * Currently provides a single-task environment with tick counting.
 * 
 * @file scheduler.h
 * @author Bengal Tiger OS (BengalEmpire)
 * @version 0.3.0
 */

#ifndef SCHEDULER_H
#define SCHEDULER_H

#include "common.h"

/* Task states */
#define TASK_STATE_READY    0
#define TASK_STATE_RUNNING  1
#define TASK_STATE_WAITING  2
#define TASK_STATE_BLOCKED  3
#define TASK_STATE_ZOMBIE   4

/* Maximum tasks (future use) */
#define MAX_TASKS 64

/* Task structure (for future multitasking) */
typedef struct task {
    uint32_t id;                /* Task ID */
    char name[32];              /* Task name */
    uint32_t state;             /* Task state */
    struct regs regs;           /* Saved CPU registers */
    uint32_t *page_dir;         /* Task's page directory */
    uint32_t kernel_stack;      /* Kernel stack pointer */
    uint32_t user_stack;        /* User stack pointer */
    struct task *next;          /* Next task in queue */
    struct task *prev;          /* Previous task in queue */
} task_t;

/* Global tick counter (from scheduler) */
extern volatile uint32_t tick;

/**
 * Initialize the scheduler.
 * Sets up the initial kernel task.
 */
void scheduler_init(void);

/**
 * Scheduler tick - called on every timer IRQ.
 * In the future, this will handle task switching.
 */
void scheduler_tick(struct regs *r);

/**
 * Get current task (stub).
 */
task_t* scheduler_get_current(void);

/**
 * Yield CPU to next task (stub - currently no-op).
 */
void scheduler_yield(void);

/**
 * Add a new task (stub - not implemented).
 */
int scheduler_add_task(task_t *task);

/**
 * Remove a task (stub - not implemented).
 */
void scheduler_remove_task(task_t *task);

/**
 * Block current task (stub - not implemented).
 */
void scheduler_block(void);

/**
 * Unblock a task (stub - not implemented).
 */
void scheduler_unblock(task_t *task);

#endif /* SCHEDULER_H */
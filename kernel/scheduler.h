/**
 * Bengal Tiger OS - Preemptive Task Scheduler
 * 
 * Supports circular multi-tasking, kernel threads, task creation, and context switching.
 * 
 * @file scheduler.h
 * @author Bengal Tiger OS (BengalEmpire)
 * @version 0.6.0
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

/* Maximum tasks */
#define MAX_TASKS 64

/* Task structure */
typedef struct task {
    uint32_t id;                /* Task ID / PID */
    char name[32];              /* Task name */
    uint32_t state;             /* Task state */
    struct regs *regs_ptr;      /* Pointer to saved register frame on task stack */
    void *stack_base;           /* Allocated stack base */
    uint32_t *page_dir;         /* Task's page directory */
    struct task *next;          /* Next task in queue */
    struct task *prev;          /* Previous task in queue */
} task_t;

/* Global tick counter */
extern volatile uint32_t tick;

/**
 * Initialize the scheduler.
 */
void scheduler_init(void);

/**
 * Scheduler tick - called on every timer IRQ for preemptive context switching.
 * Returns pointer to the current task's register frame.
 */
struct regs* scheduler_tick(struct regs *r);

/**
 * Get current task.
 */
task_t* scheduler_get_current(void);

/**
 * Yield CPU to next task.
 */
void scheduler_yield(void);

/**
 * Create a new kernel task thread.
 */
task_t* task_create(const char *name, void (*entry_point)(void));

/**
 * Terminate current task execution.
 */
void task_exit(void);

/**
 * Kill task by PID.
 */
int task_kill(uint32_t pid);

/**
 * Get list of tasks.
 */
task_t* scheduler_get_task_list(int *count);

#endif /* SCHEDULER_H */

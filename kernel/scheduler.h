/**
 * Bengal Tiger OS - Preemptive Task Scheduler
 *
 * Round-robin preemptive multitasking kernel thread scheduler with
 * per-task CPU tick accounting, clean voluntary yields (INT 0x80),
 * zombie reaping, and interrupt-safe task creation/termination.
 *
 * @file scheduler.h
 * @author Bengal Tiger OS (BengalEmpire)
 * @version 1.0.0
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

#define MAX_TASKS 64

typedef struct task {
    uint32_t id;                /* Task ID / PID */
    char name[32];              /* Task name */
    volatile uint32_t state;    /* Task state */
    uint32_t cpu_ticks;         /* Scheduled CPU ticks */
    struct regs *regs_ptr;      /* Saved register frame on task stack */
    void *stack_base;           /* Allocated kernel stack base */
    uint32_t *page_dir;         /* Task page directory (NULL = kernel) */
    struct task *next;
    struct task *prev;
} task_t;

extern volatile uint32_t tick;

void scheduler_init(void);
struct regs* scheduler_tick(struct regs *r);
struct regs* scheduler_schedule(struct regs *r);
task_t* scheduler_get_current(void);
void scheduler_yield(void);
task_t* task_create(const char *name, void (*entry_point)(void));
void task_exit(void);
int task_kill(uint32_t pid);
task_t* scheduler_get_task_list(int *count);

#endif /* SCHEDULER_H */

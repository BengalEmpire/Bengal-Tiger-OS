/**
 * Bengal Tiger OS - Preemptive Task Scheduler Implementation
 *
 * Implements circular linked list task queue, thread creation, context switching,
 * and task lifecycle management.
 * 
 * @file scheduler.c
 * @author Bengal Tiger OS (BengalEmpire)
 * @version 0.6.0
 */

#include "scheduler.h"
#include "common.h"
#include "heap.h"
#include "serial.h"

#define STACK_SIZE 8192

volatile uint32_t tick = 0;

static task_t kernel_task;
static task_t *current_task = NULL;
static task_t *task_head = NULL;
static uint32_t next_pid = 1;

void scheduler_init(void) {
    memset(&kernel_task, 0, sizeof(task_t));
    kernel_task.id = 0;
    strcpy(kernel_task.name, "kernel");
    kernel_task.state = TASK_STATE_RUNNING;
    kernel_task.next = &kernel_task;
    kernel_task.prev = &kernel_task;

    current_task = &kernel_task;
    task_head = &kernel_task;
}

task_t* task_create(const char *name, void (*entry_point)(void)) {
    task_t *t = (task_t*)kmalloc(sizeof(task_t));
    if (!t) return NULL;

    memset(t, 0, sizeof(task_t));
    t->id = next_pid++;
    strncpy(t->name, name, 31);
    t->state = TASK_STATE_READY;

    void *stack = kmalloc(STACK_SIZE);
    if (!stack) {
        kfree(t);
        return NULL;
    }
    memset(stack, 0, STACK_SIZE);
    t->stack_base = stack;

    /* Setup initial register frame on task stack for irq_common restore */
    uint32_t *top = (uint32_t*)((uint32_t)stack + STACK_SIZE);

    /* Return address for task if function returns */
    top--; *top = (uint32_t)task_exit;

    /* Fake iret frame: eflags, cs, eip */
    top--; *top = 0x202;                /* eflags (IF=1) */
    top--; *top = 0x08;                 /* cs */
    top--; *top = (uint32_t)entry_point;/* eip */

    /* pusha frame: eax, ecx, edx, ebx, esp, ebp, esi, edi */
    top--; *top = 0;                    /* err_code */
    top--; *top = 0;                    /* int_no */
    top--; *top = 0;                    /* eax */
    top--; *top = 0;                    /* ecx */
    top--; *top = 0;                    /* edx */
    top--; *top = 0;                    /* ebx */
    uint32_t stack_esp = (uint32_t)top;
    top--; *top = stack_esp;            /* esp */
    top--; *top = stack_esp;            /* ebp */
    top--; *top = 0;                    /* esi */
    top--; *top = 0;                    /* edi */

    /* segment selectors: ds, es, fs, gs */
    top--; *top = 0x10;                 /* ds */
    top--; *top = 0x10;                 /* es */
    top--; *top = 0x10;                 /* fs */
    top--; *top = 0x10;                 /* gs */

    t->regs_ptr = (struct regs*)top;

    /* Insert into circular doubly linked list */
    t->next = task_head;
    t->prev = task_head->prev;
    task_head->prev->next = t;
    task_head->prev = t;

    serial_write_str("Scheduler: Created task ");
    serial_write_str(t->name);
    serial_write_str("\n");

    return t;
}

void task_exit(void) {
    if (!current_task || current_task->id == 0) {
        while (1) { __asm__ volatile("hlt"); }
    }

    current_task->state = TASK_STATE_ZOMBIE;
    scheduler_yield();

    while (1) { __asm__ volatile("hlt"); }
}

int task_kill(uint32_t pid) {
    if (pid == 0) return -1;

    task_t *t = task_head;
    if (!t) return -1;

    do {
        if (t->id == pid) {
            t->state = TASK_STATE_ZOMBIE;
            return 0;
        }
        t = t->next;
    } while (t != task_head);

    return -1;
}

task_t* scheduler_get_current(void) {
    return current_task;
}

task_t* scheduler_get_task_list(int *count) {
    if (!task_head) {
        if (count) *count = 0;
        return NULL;
    }

    int c = 0;
    task_t *curr = task_head;
    do {
        c++;
        curr = curr->next;
    } while (curr != task_head);

    if (count) *count = c;
    return task_head;
}

struct regs* scheduler_tick(struct regs *r) {
    tick++;

    if (!current_task) return r;

    /* Save register pointer for current task */
    current_task->regs_ptr = r;

    /* Clean up zombie tasks */
    task_t *curr = task_head->next;
    while (curr != task_head) {
        task_t *next = curr->next;
        if (curr->state == TASK_STATE_ZOMBIE && curr != current_task) {
            curr->prev->next = curr->next;
            curr->next->prev = curr->prev;
            if (curr->stack_base) kfree(curr->stack_base);
            kfree(curr);
        }
        curr = next;
    }

    /* Select next task */
    task_t *next_task = current_task->next;
    while (next_task != current_task && next_task->state != TASK_STATE_READY && next_task->state != TASK_STATE_RUNNING) {
        next_task = next_task->next;
    }

    if (next_task == current_task && current_task->state != TASK_STATE_RUNNING && current_task->state != TASK_STATE_READY) {
        next_task = &kernel_task;
    }

    if (next_task != current_task) {
        if (current_task->state == TASK_STATE_RUNNING) {
            current_task->state = TASK_STATE_READY;
        }

        current_task = next_task;
        current_task->state = TASK_STATE_RUNNING;
    }

    return current_task->regs_ptr;
}

void scheduler_yield(void) {
    __asm__ volatile("int $32");
}

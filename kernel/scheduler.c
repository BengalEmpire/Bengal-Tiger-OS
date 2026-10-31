/**
 * Bengal Tiger OS - Preemptive Task Scheduler Implementation
 *
 * @file scheduler.c
 * @author Bengal Tiger OS (BengalEmpire)
 * @version 1.0.0
 */

#include "scheduler.h"
#include "common.h"
#include "heap.h"
#include "klog.h"

#define STACK_SIZE 8192

volatile uint32_t tick = 0;

static task_t kernel_task;
static task_t *current_task = NULL;
static task_t *task_head    = NULL;
static uint32_t next_pid    = 1;

void scheduler_init(void) {
    uint32_t irq = irq_save();

    memset(&kernel_task, 0, sizeof(task_t));
    kernel_task.id    = 0;
    strcpy(kernel_task.name, "kernel");
    kernel_task.state = TASK_STATE_RUNNING;
    kernel_task.next  = &kernel_task;
    kernel_task.prev  = &kernel_task;

    current_task = &kernel_task;
    task_head    = &kernel_task;
    next_pid     = 1;

    irq_restore(irq);
}

task_t* task_create(const char *name, void (*entry_point)(void)) {
    if (!entry_point || !task_head) return NULL;

    uint32_t irq = irq_save();

    /* Enforce MAX_TASKS */
    int count = 0;
    scheduler_get_task_list(&count);
    if (count >= MAX_TASKS) {
        irq_restore(irq);
        return NULL;
    }

    task_t *t = (task_t*)kzalloc(sizeof(task_t));
    if (!t) {
        irq_restore(irq);
        return NULL;
    }

    void *stack = kzalloc(STACK_SIZE);
    if (!stack) {
        kfree(t);
        irq_restore(irq);
        return NULL;
    }

    t->id = next_pid++;
    strncpy(t->name, name ? name : "thread", 31);
    t->name[31]   = '\0';
    t->state      = TASK_STATE_READY;
    t->cpu_ticks  = 0;
    t->stack_base = stack;

    /* Align stack top to 16 bytes for System V i386 ABI */
    uint32_t *top = (uint32_t*)ALIGN_DOWN((uint32_t)stack + STACK_SIZE, 16);

    /* Return address when entry_point() returns */
    top--; *top = (uint32_t)task_exit;

    /* Hardware iret frame (ring 0 -> ring 0): eflags, cs, eip */
    top--; *top = 0x00000202U;          /* EFLAGS (IF=1, reserved bit 1=1) */
    top--; *top = 0x08U;                /* CS (kernel code) */
    top--; *top = (uint32_t)entry_point;/* EIP */

    /* ISR stub frame: err_code, int_no */
    top--; *top = 0;
    top--; *top = 0;

    /* pusha frame: eax, ecx, edx, ebx, esp, ebp, esi, edi */
    top--; *top = 0;                    /* eax */
    top--; *top = 0;                    /* ecx */
    top--; *top = 0;                    /* edx */
    top--; *top = 0;                    /* ebx */
    uint32_t stack_esp = (uint32_t)top;
    top--; *top = stack_esp;            /* esp */
    top--; *top = stack_esp;            /* ebp */
    top--; *top = 0;                    /* esi */
    top--; *top = 0;                    /* edi */

    /* Segment registers: ds, es, fs, gs */
    top--; *top = 0x10U;
    top--; *top = 0x10U;
    top--; *top = 0x10U;
    top--; *top = 0x10U;

    t->regs_ptr = (struct regs*)top;

    /* Insert at tail of circular doubly-linked list */
    t->next = task_head;
    t->prev = task_head->prev;
    task_head->prev->next = t;
    task_head->prev = t;

    irq_restore(irq);

    klog_writeln("sched", t->name);
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

    uint32_t irq = irq_save();
    if (!task_head) {
        irq_restore(irq);
        return -1;
    }

    task_t *t = task_head;
    do {
        if (t->id == pid && t->state != TASK_STATE_ZOMBIE) {
            t->state = TASK_STATE_ZOMBIE;
            irq_restore(irq);
            return 0;
        }
        t = t->next;
    } while (t != task_head);

    irq_restore(irq);
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

struct regs* scheduler_schedule(struct regs *r) {
    if (!current_task || !task_head) return r;

    current_task->regs_ptr = r;

    /* Reap zombie tasks (other than the currently executing stack!) */
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

    /* Round-robin selection */
    task_t *next_task = current_task->next;
    while (next_task != current_task &&
           next_task->state != TASK_STATE_READY &&
           next_task->state != TASK_STATE_RUNNING) {
        next_task = next_task->next;
    }

    if (next_task == current_task &&
        current_task->state != TASK_STATE_RUNNING &&
        current_task->state != TASK_STATE_READY) {
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

struct regs* scheduler_tick(struct regs *r) {
    tick++;
    if (current_task) {
        current_task->cpu_ticks++;
    }
    return scheduler_schedule(r);
}

void scheduler_yield(void) {
    /* Invoke SYS_YIELD (158) via INT 0x80 so PIC EOI & PIT ticks aren't perturbed */
    __asm__ volatile(
        "mov $158, %%eax\n\t"
        "int $0x80"
        :
        :
        : "eax", "memory"
    );
}

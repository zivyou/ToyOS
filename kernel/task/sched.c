#include "task/task.h"
#include "task/sched.h"
#include "printk.h"
#include "common.h"
#include "task/idle.h"

// Global idle task (PID 0)
task_t* idle_task = NULL;

// Task to switch to (set by schedule, checked by interrupt handler)
volatile task_t* task_to_switch = NULL;

_Noreturn void entry() {
    printk("--------------------------->\n");
    // __asm__ volatile (
    //     "int $0x80"
    // );
    while (1) {
        printk("in entry.........................>\n");
        hlt();                                                                                                                 
    }  
}

/**
 * scheduler_init - Initialize the scheduler
 *
 * Creates the idle task (PID 0) and switches to it.
 * After this function returns, the system is in scheduling mode.
 */
void scheduler_init(void) {
    printk("scheduler_init: Round-Robin scheduler initialized with time slice %d\n",
           DEFAULT_TIME_SLICE);

    // Create idle task as PID 0 (the system's guard task)
    idle_task = task_create(idle, 1);

    if (!idle_task) {
        printk("scheduler_init: FATAL - Failed to create idle task!\n");
        while (1) { hlt(); }  // Hang forever
    }

    task_create(entry, 1);

    printk("scheduler_init: Switching to idle task (PID %d)\n", idle_task->pid);

    // Switch to idle task - from this point on, current is always valid
    switch_task(idle_task);

    // This code should never execute as idle is an infinite loop
    printk("scheduler_init: ERROR - Returned from idle task! System hang.\n");
    while (1) { hlt(); }
}

/**
 * pick_next_task - Select the next task to run
 *
 * Returns: Pointer to the next task to run, or NULL if no tasks available
 */
task_t* pick_next_task(void) {
    if (!current) {
        // No current task, return head of list
        return task_list_head;
    }

    // Find next READY task in the list
    task_t* next = current->next;

    // If no next task, wrap around to head
    if (!next) {
        next = task_list_head;
    }

    // Skip non-READY tasks
    while (next && next != current) {
        if (next->state == TASK_READY) {
            return next;
        }
        next = next->next;
        if (!next) {
            next = task_list_head;
        }
    }

    // If we're back to current task, check if it's still READY
    if (current->state == TASK_READY) {
        return current;
    }

    // No READY task found
    return NULL;
}

/**
 * schedule - Main scheduler function
 *
 * Called from timer interrupt to perform task switching.
 * Sets task_to_switch flag; actual switching is done by interrupt handler.
 */
void schedule(void) {
    if (current == NULL) return;

    // Decrement current task's time slice
    if (current && current->state == TASK_RUNNING) {
        current->time_slice--;

        if (current->time_slice > 0) {
            // Task still has time, don't switch
            return;
        }

        // Time slice exhausted, reset it
        current->time_slice = DEFAULT_TIME_SLICE;
    }

    // Pick next task
    task_t* next = pick_next_task();

    if (!next) {
        // No tasks to run, stay on current
        task_to_switch = NULL;
        return;
    }

    if (next != current) {
        // Set task to switch - will be handled by interrupt handler
        printk("schedule: Will switch from task %d to task %d\n",
               current ? current->pid : 0, next->pid);
        task_to_switch = next;
    } else {
        task_to_switch = NULL;
    }
}

/**
 * need_reschedule - Force a reschedule
 *
 * Called from sys_yield or when a task voluntarily gives up CPU
 */
void need_reschedule(void) {
    // Reset current task's time slice to force reschedule
    if (current) {
        current->time_slice = 0;
    }
}

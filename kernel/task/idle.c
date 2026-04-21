#include "task/task.h"
#include "task/sched.h"
#include "common.h"
#include "printk.h"

/**
 * idle - The idle task
 *
 * This is the task that runs when no other task is ready to run.
 * It continuously tries to switch to the next ready task or halts the CPU.
 */
_Noreturn void idle() {
    printk("idle: Idle task started\n");

    while (1) {
        // Try to find a next task to run
        task_t* next = pick_next_task();

        if (next && next != current) {
            // Switch to the next task
            switch_task(next);
        } else {
            // No other tasks to run, halt the CPU until next interrupt
            hlt();
        }
    }
}

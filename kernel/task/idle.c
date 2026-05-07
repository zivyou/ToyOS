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
        // Run scheduler to check for task switches
        // This is called after timer interrupts set time_slice = 0
        schedule();
        printk("in idle loop...............\n");
        // If we're still on idle task after schedule, no other tasks are ready
        if (current == idle_task) {
            // Halt the CPU until next interrupt
            hlt();
        }
        // Otherwise, we switched to another task, and will come back here when needed
    }
}

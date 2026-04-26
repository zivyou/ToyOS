#include "task/task.h"
#include "task/sched.h"
#include "common.h"
#include "printk.h"

/**
 * idle - The idle task
 *
 * This is the task that runs when no other task is ready to run.
 * It halts the CPU until the next timer interrupt triggers a task switch.
 */
_Noreturn void idle() {
    printk("idle: Idle task started\n");

    while (1) {
        // Halt the CPU until next interrupt
        // Timer interrupt will call schedule() and trigger task switch
        hlt();
        printk("idle: woke up, still idle...\n");
    }
}

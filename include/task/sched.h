#ifndef SCHED_H
#define SCHED_H

#include "types.h"

// Default time slice (in timer ticks)
// Assuming 100 Hz timer, this gives 10ms per task
#define DEFAULT_TIME_SLICE 10

// Global idle task (PID 0, always exists)
extern task_t* idle_task;

// Scheduler initialization
void scheduler_init(void);

// Main scheduler function - called from timer interrupt
void schedule(void);

// Pick the next task to run
task_t* pick_next_task(void);

// Force a reschedule (called from sys_yield, etc.)
void need_reschedule(void);

#endif // SCHED_H

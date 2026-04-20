#include "task/task.h"
#include "gdt/tss.h"
#include "printk.h"
#include "mm/heap.h"
#include "mm/mm.h"

// Static PID counter
static uint32_t next_pid = 1;

// Create a new task
task_t* task_create(void (*entry)(void), uint32_t is_kernel_task) {
    // Allocate task structure
    task_t *task = kmalloc(sizeof(task_t));
    if (!task) {
        printk("task_create: Failed to allocate task structure\n");
        return NULL;
    }

    // Initialize task fields
    task->pid = next_pid++;
    task->state = TASK_READY;
    task->entry = entry;
    task->next = NULL;
    task->prev = NULL;

    // Allocate kernel stack
    task->kernel_stack_size = KERNEL_STACK_SIZE;
    task->kernel_stack = (uint32_t)kmalloc(KERNEL_STACK_SIZE);
    if (!task->kernel_stack) {
        printk("task_create: Failed to allocate kernel stack\n");
        kfree(task);
        return NULL;
    }

    // Set kernel stack to top of allocated memory
    task->context.esp = task->kernel_stack + KERNEL_STACK_SIZE;

    // For user tasks, allocate user stack
    if (!is_kernel_task) {
        task->user_stack_size = USER_STACK_SIZE;
        task->user_stack = (uint32_t)kmalloc(USER_STACK_SIZE);
        if (!task->user_stack) {
            printk("task_create: Failed to allocate user stack\n");
            kfree((void*)task->kernel_stack);
            kfree(task);
            return NULL;
        }
    } else {
        task->user_stack = 0;
        task->user_stack_size = 0;
    }

    // Initialize context
    task->context.eip = (uint32_t)entry;
    task->context.eflags = 0x200;  // Enable interrupts
    task->context.edi = 0;
    task->context.esi = 0;
    task->context.ebp = 0;
    task->context.ebx = 0;
    task->context.edx = 0;
    task->context.ecx = 0;
    task->context.eax = 0;

    printk("task_create: Created task %d with entry 0x%x\n",
           task->pid, (uint32_t)entry);

    return task;
}

// Destroy a task
void task_destroy(task_t *task) {
    if (!task) {
        return;
    }

    printk("task_destroy: Destroying task %d\n", task->pid);

    // Free kernel stack
    if (task->kernel_stack) {
        kfree((void*)task->kernel_stack);
    }

    // Free user stack
    if (task->user_stack) {
        kfree((void*)task->user_stack);
    }

    // Free task structure
    kfree(task);
}

// Set task state
void task_set_state(task_t *task, task_state_t state) {
    if (task) {
        task->state = state;
    }
}

// Get task state
task_state_t task_get_state(task_t *task) {
    if (task) {
        return task->state;
    }
    return TASK_EXITED;
}

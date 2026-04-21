#include "task/task.h"
#include "task/sched.h"
#include "gdt/tss.h"
#include "printk.h"
#include "mm/heap.h"
#include "mm/mm.h"

// Static PID counter (starts from 0 for idle task)
static uint32_t next_pid = 0;

// task list
task_t* task_list_head = NULL;
task_t* task_list_tail = NULL;

task_t* current = NULL;

void add_to_task_list(task_t * task) {
    task->prev = task_list_tail;
    task->next = NULL;

    if (task_list_head == NULL) {
        // 第一个节点
        task_list_head = task;
        task_list_tail = task;
    } else {
        task_list_tail->next = task;
        task_list_tail = task;
    }
}

task_t* remove_from_task_list(task_t * task) {
    if (!task) return NULL;

    // 从链表中移除
    if (task->prev) {
        task->prev->next = task->next;
    } else {
        // task是头节点
        task_list_head = task->next;
    }

    if (task->next) {
        task->next->prev = task->prev;
    } else {
        // task是尾节点
        task_list_tail = task->prev;
    }

    task->prev = NULL;
    task->next = NULL;
    return task;
}

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
    task->time_slice = DEFAULT_TIME_SLICE;
    task->priority = 0;

    // Allocate kernel stack
    task->kernel_stack_size = KERNEL_STACK_SIZE;
    task->kernel_stack = (uint32_t)kmalloc(KERNEL_STACK_SIZE);
    if (!task->kernel_stack) {
        printk("task_create: Failed to allocate kernel stack\n");
        kfree(task);
        return NULL;
    }

    // Pre-stack context data for task switching
    // Stack layout (from high to low address, stack grows down):
    //
    //   [higher addresses...]
    //   [entry]      - Return address for ret
    //   [0]          - saved EBP
    //   [0]          - saved EDI
    //   [0]          - saved ESI
    //   [0x202]      - EFLAGS (interrupts enabled)
    //   [lower addresses...]
    //
    // When switch_task is called:
    //   - We pop ESI, EDI, EBP, EFLAGS (restoring them)
    //   - Then jmp jumps to entry point

    uint32_t *stack_top = (uint32_t*)(task->kernel_stack + KERNEL_STACK_SIZE);

    // Saved registers (will be restored in switch_task)
    *(--stack_top) = 0x202;            // EFLAGS (IF=1, IOPL=0 - interrupts enabled)
    *(--stack_top) = 0;                // ESI
    *(--stack_top) = 0;                // EDI
    *(--stack_top) = 0;                // EBP

    // Return address for ret
    *(--stack_top) = (uint32_t)entry;  // Task entry point

    task->context.esp = (uint32_t)stack_top;

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

    printk("task_create: Created task %d with entry 0x%x\n",
           task->pid, (uint32_t)entry);
    
    add_to_task_list(task);
    return task;
}

// External idle task reference
extern task_t* idle_task;

// Destroy a task
task_t* task_destroy(task_t *task) {
    if (!task) {
        return NULL;
    }

    // Cannot destroy the idle task
    if (task == idle_task) {
        printk("task_destroy: Cannot destroy idle task (PID %d)!\n", task->pid);
        return NULL;
    }

    if (task->state != TASK_EXITED) {
        printk("task_destroy failed! task state: %d\n", task->state);
        return NULL;
    }

    printk("task_destroy: Destroying task %d\n", task->pid);

    // 先移除链表引用
    task_t* ret = remove_from_task_list(task);

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
    task = NULL;
    return ret;
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

void switch_task(task_t * task) {
    if (!task || task == current) return;

    // Save current task state
    if (current) {
        task_set_state(current, TASK_READY);

        __asm__ volatile(
            "pushl %%esi\n"     // Push ESI
            "pushl %%edi\n"     // Push EDI
            "pushl %%ebp\n"     // Push EBP
            "pushfl\n"          // Push EFLAGS (important for interrupt state)
            "movl %%esp, %0\n"  // Save ESP
            : "=m" (current->context.esp)
            :
            : "memory"
        );
    }

    // Switch to new task
    task_set_state(task, TASK_RUNNING);
    current = task;

    // Restore new task context and jump to entry point
    // Stack layout: [ret_addr][EBP][EDI][ESI][EFLAGS]
    // ESP points to ret_addr
    __asm__ volatile(
        "movl %0, %%esp\n"  // Load new task's ESP (points to ret_addr)
        "popl %%eax\n"      // Pop ret_addr into EAX
        "popl %%esi\n"      // Restore ESI
        "popl %%edi\n"      // Restore EDI
        "popl %%ebp\n"      // Restore EBP
        "popfl\n"           // Restore EFLAGS
        "jmp *%%eax\n"      // Jump to entry point
        :
        : "r" (task->context.esp)
        : "eax", "memory"
    );
}

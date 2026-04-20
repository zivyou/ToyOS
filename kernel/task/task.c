#include "task/task.h"
#include "gdt/tss.h"
#include "printk.h"
#include "mm/heap.h"
#include "mm/mm.h"

// Static PID counter
static uint32_t next_pid = 1;

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

    // Initialize context on the stack for first-time task switch
    uint32_t* stack_top = (uint32_t*)(task->kernel_stack + KERNEL_STACK_SIZE);

    // Push entry point as return address for 'ret' instruction
    // This will be the FIRST thing on the stack, so ret can pop it after popal and popfl
    *--stack_top = (uint32_t)entry;

    // Push eflags with interrupts enabled (this will be popped by popfl)
    *--stack_top = 0x202;  // IF + reserved bit 1

    // Push general registers (pushal pushes: eax, ecx, edx, ebx, esp, ebp, esi, edi)
    // So we need to set them up in the same order for popal
    *--stack_top = 0;  // eax
    *--stack_top = 0;  // ecx
    *--stack_top = 0;  // edx
    *--stack_top = 0;  // ebx
    *--stack_top = (uint32_t)stack_top;  // esp (ignored by popal)
    *--stack_top = 0;  // ebp
    *--stack_top = 0;  // esi
    *--stack_top = 0;  // edi

    // Update context.esp to point to the prepared stack (top of registers)
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

// Destroy a task
task_t* task_destroy(task_t *task) {
    if (!task) {
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
            "pushfl\n"         // Push EFLAGS
            "pushal\n"         // Push all general registers
            "movl %%esp, %0\n" // Save ESP to current->context.esp
            : "=m" (current->context.esp)
            :
            : "memory"
        );
    }

    // Switch to new task
    task_set_state(task, TASK_RUNNING);
    current = task;

    // Restore new task context
    __asm__ volatile(
        "movl %0, %%esp\n"  // Load new task's ESP
        "popal\n"           // Restore general registers
        "popfl\n"           // Restore EFLAGS
        "ret\n"             // Return to task entry point
        :
        : "r" (task->context.esp)
        : "memory"
    );
}

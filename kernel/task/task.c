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

    // Pre-stack interrupt frame for task switching.
    // Layout matches what common_irq pushes / iret expects:
    //
    //   High address (stack base + KERNEL_STACK_SIZE)
    //   EFLAGS       ← iret pops this
    //   CS           ← iret pops this
    //   EIP          ← iret pops this (= entry point)
    //   error_code   ← addl $8 removes this
    //   int_no       ← addl $8 removes this
    //   pusha frame  ← popa restores (EAX,ECX,EDX,EBX,ESP,EBP,ESI,EDI)
    //   saved DS     ← popl %ebx then mov %bx,%ds etc.
    //   Low address  ← ESP points here

    uint32_t *stack_top = (uint32_t*)(task->kernel_stack + KERNEL_STACK_SIZE);

    // IRET frame
    *(--stack_top) = 0x202;            // EFLAGS (IF=1)
    *(--stack_top) = 0x08;             // CS (kernel code segment)
    *(--stack_top) = (uint32_t)entry;  // EIP (entry point)

    // IRQ handler pushes
    *(--stack_top) = 0;                // error_code
    *(--stack_top) = 0;                // int_no

    // pusha frame (order: EAX, ECX, EDX, EBX, ESP, EBP, ESI, EDI)
    *(--stack_top) = 0;                // EDI
    *(--stack_top) = 0;                // ESI
    *(--stack_top) = 0;                // EBP
    *(--stack_top) = 0;                // ESP (ignored by popa)
    *(--stack_top) = 0;                // EBX
    *(--stack_top) = 0;                // EDX
    *(--stack_top) = 0;                // ECX
    *(--stack_top) = 0;                // EAX

    // Saved DS segment
    *(--stack_top) = 0x10;             // DS = kernel data segment

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

    // Only used for the initial switch in scheduler_init().
    // No need to save old context — scheduler_init never returns.

    task_set_state(task, TASK_RUNNING);
    current = task;

    // Update TSS esp0
    tss_set_kernel_stack(0x10, task->kernel_stack + task->kernel_stack_size);

    // Restore from interrupt frame layout (same as common_irq restore path)
    __asm__ volatile(
        "movl %0, %%esp\n"     // Load new task's ESP
        "popl %%ebx\n"         // Pop saved DS
        "mov %%bx, %%ds\n"
        "mov %%bx, %%ss\n"
        "mov %%bx, %%es\n"
        "mov %%bx, %%fs\n"
        "mov %%bx, %%gs\n"
        "popa\n"               // Restore all general registers
        "addl $8, %%esp\n"     // Remove int_no and error_code
        "iret\n"               // Pop EIP, CS, EFLAGS → jump to entry
        :
        : "r" (task->context.esp)
        : "memory"
    );
    __builtin_unreachable();
}

#include "task/task.h"
#include "task/sched.h"
#include "gdt/tss.h"
#include "printk.h"
#include "mm/heap.h"
#include "mm/mm.h"

// Simple memset implementation
static void* memset(void* s, int c, uint32_t n) {
    unsigned char* p = (unsigned char*)s;
    while (n--) {
        *p++ = (unsigned char)c;
    }
    return s;
}

// Static PID counter (starts from 0 for idle task)
static uint32_t next_pid = 0;

// task list
task_t* task_list_head = NULL;
task_t* task_list_tail = NULL;

// current running task
task_t* current = NULL;

// Task array (for indexed access, like Linux 0.11)
task_t* task[NR_TASKS] = {NULL};

// First TSS index in GDT (after the 5 base entries: NULL, KTEXT, KDATA, UTEXT, UDATA)
#define FIRST_TSS_INDEX 5

// External GDT functions
extern void set_gdt_entry_for_tss(int32_t index, uint32_t base, uint32_t limit, uint32_t access, uint32_t gran);
extern void gdt_flush(uint32_t);

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
    task_t *new_task = kmalloc(sizeof(task_t));
    if (!new_task) {
        printk("task_create: Failed to allocate task structure\n");
        return NULL;
    }

    // Initialize task fields
    new_task->pid = next_pid++;
    new_task->state = TASK_READY;
    new_task->entry = entry;
    new_task->next = NULL;
    new_task->prev = NULL;
    new_task->time_slice = DEFAULT_TIME_SLICE;
    new_task->priority = 0;

    // Allocate kernel stack
    new_task->kernel_stack_size = KERNEL_STACK_SIZE;
    new_task->kernel_stack = (uint32_t)kmalloc(KERNEL_STACK_SIZE);
    if (!new_task->kernel_stack) {
        printk("task_create: Failed to allocate kernel stack\n");
        kfree(new_task);
        return NULL;
    }

    // Clear TSS structure
    memset(&new_task->tss, 0, sizeof(tss_entry_t));

    // Set up TSS for the task
    // Kernel segments
    new_task->tss.ts_cs = 0x08;   // Kernel code segment
    new_task->tss.ts_ds = 0x10;   // Kernel data segment
    new_task->tss.ts_es = 0x10;
    new_task->tss.ts_ss = 0x10;
    new_task->tss.ts_fs = 0x10;
    new_task->tss.ts_gs = 0x10;

    // Set up kernel stack (esp0 and ss0 for privilege transitions)
    new_task->tss.ts_esp0 = new_task->kernel_stack + KERNEL_STACK_SIZE;
    new_task->tss.ts_ss0 = 0x10;

    // Set entry point
    new_task->tss.ts_eip = (uint32_t)entry;

    // Set up ESP (task's own stack pointer)
    new_task->tss.ts_esp = new_task->kernel_stack + KERNEL_STACK_SIZE;

    // Set EFLAGS (enable interrupts)
    new_task->tss.ts_eflags = 0x202;  // IF=1, IOPL=0

    // Clear general registers
    new_task->tss.ts_eax = 0;
    new_task->tss.ts_ecx = 0;
    new_task->tss.ts_edx = 0;
    new_task->tss.ts_ebx = 0;
    new_task->tss.ts_ebp = 0;
    new_task->tss.ts_esi = 0;
    new_task->tss.ts_edi = 0;

    // Important: Set I/O map base to indicate no I/O map
    // Setting it to sizeof(tss_entry_t) means no I/O permission map
    new_task->tss.ts_iomb = sizeof(tss_entry_t);
    // Clear trap flag (no trap on task switch)
    new_task->tss.ts_t = 0;

    // Clear link field (will be set by CPU during task switch)
    new_task->tss.ts_link = 0;

    // Set up TSS descriptor in GDT
    // Find a free TSS slot
    uint32_t tss_index = 0;
    int task_index = -1;
    for (int i = 0; i < NR_TASKS; i++) {
        if (task[i] == NULL) {
            tss_index = FIRST_TSS_INDEX + i;  // Each task needs 1 GDT entry for TSS
            task_index = i;
            task[i] = new_task;
            break;
        }
    }

    if (task_index < 0) {
        printk("task_create: No free task slots!\n");
        kfree((void*)new_task->kernel_stack);
        kfree(new_task);
        return NULL;
    }

    new_task->tss_gdt_index = tss_index;
    new_task->tss_selector = tss_index << 3;  // Selector = index * 8

    // Set up TSS descriptor in GDT
    // Base = address of TSS structure
    // Limit = sizeof(tss_entry_t) - 1
    // Access = 0x89 (present, DPL=0, TSS type=0x9)
    // Granularity = 0x00 (byte granularity)
    uint32_t tss_base = (uint32_t)&new_task->tss;
    uint32_t tss_limit = sizeof(tss_entry_t) - 1;
    set_gdt_entry_for_tss(tss_index, tss_base, tss_limit, 0x89, 0x00);

    // For user tasks, allocate user stack
    if (!is_kernel_task) {
        new_task->user_stack_size = USER_STACK_SIZE;
        new_task->user_stack = (uint32_t)kmalloc(USER_STACK_SIZE);
        if (!new_task->user_stack) {
            printk("task_create: Failed to allocate user stack\n");
            kfree((void*)new_task->kernel_stack);
            // Remove from task array
            task[task_index] = NULL;
            kfree(new_task);
            return NULL;
        }
        // For user tasks, we'd set up user segments here
        // But for now, just keep kernel segments
    } else {
        new_task->user_stack = 0;
        new_task->user_stack_size = 0;
    }

    printk("task_create: Created task %d with entry 0x%x, TSS selector 0x%x\n",
           new_task->pid, (uint32_t)entry, new_task->tss_selector);

    add_to_task_list(new_task);
    return new_task;
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

    // Switch to new task FIRST (before ljmp)
    // This ensures current is valid when CPU performs the task switch
    task_t* prev = current;
    task_set_state(task, TASK_RUNNING);
    current = task;

    // Hardware task switch using ljmp to TSS
    // This instruction causes the CPU to:
    // 1. Save all registers to the PREVIOUS task's TSS (if prev != NULL)
    // 2. Load all registers from the NEW task's TSS
    // 3. Jump to the new task's EIP
    // Note: If prev is NULL (initial boot), CPU skips step 1
    __asm__ volatile(
        "ljmp *%0"
        :
        : "m" (task->tss_selector)
    );

    // Never reached (ljmp doesn't return)
}

// Get TSS selector for a task (used by interrupt handler)
uint32_t task_get_tss_selector(task_t *task) {
    if (task) {
        return task->tss_selector;
    }
    return 0;
}

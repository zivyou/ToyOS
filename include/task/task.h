#ifndef TASK_H
#define TASK_H

#include "types.h"

// Maximum number of tasks
#define NR_TASKS 64

// Task states
typedef enum task_state {
    TASK_READY,      // Ready to run, waiting for CPU
    TASK_RUNNING,    // Currently running on CPU
    TASK_BLOCKED,    // Blocked waiting for I/O or resource
    TASK_ZOMBIE,     // Task finished, waiting to be reaped
    TASK_EXITED      // Task has been cleaned up
} task_state_t;

// Task State Segment (TSS) for hardware task switching
typedef struct tss_entry {
    uint32_t  ts_link;   // old ts selector (back link)
    uint32_t  ts_esp0;   // stack pointer for privilege level 0
    uint32_t  ts_ss0;    // stack segment for privilege level 0
    uint32_t  ts_esp1;   // stack pointer for privilege level 1
    uint32_t  ts_ss1;    // stack segment for privilege level 1
    uint32_t  ts_esp2;   // stack pointer for privilege level 2
    uint32_t  ts_ss2;    // stack segment for privilege level 2
    uint32_t  ts_cr3;    // page directory base
    uint32_t  ts_eip;    // instruction pointer
    uint32_t  ts_eflags; // flags register
    uint32_t  ts_eax;    // general registers
    uint32_t  ts_ecx;
    uint32_t  ts_edx;
    uint32_t  ts_ebx;
    uint32_t  ts_esp;    // stack pointer
    uint32_t  ts_ebp;    // base pointer
    uint32_t  ts_esi;    // source index
    uint32_t  ts_edi;    // destination index
    uint32_t  ts_es;     // segment registers
    uint32_t  ts_cs;
    uint32_t  ts_ss;
    uint32_t  ts_ds;
    uint32_t  ts_fs;
    uint32_t  ts_gs;
    uint32_t  ts_ldt;    // local descriptor table
    uint16_t  ts_t;      // trap on task switch
    uint16_t  ts_iomb;   // I/O map base address
} __attribute__((packed)) tss_entry_t;

// Process Control Block (PCB)
typedef struct task {
    uint32_t pid;                  // Process ID
    task_state_t state;            // Task state
    uint32_t kernel_stack;         // Kernel stack top address
    uint32_t kernel_stack_size;    // Kernel stack size (in bytes)

    // Task State Segment for hardware task switching
    tss_entry_t tss;               // TSS structure
    uint32_t tss_selector;         // TSS selector in GDT
    uint32_t tss_gdt_index;        // GDT index for this task's TSS

    // For user tasks
    uint32_t user_stack;           // User stack top address
    uint32_t user_stack_size;      // User stack size (in bytes)

    // Task entry point
    void (*entry)(void);           // Task entry function

    // For linked list management
    struct task *next;             // Next task in list
    struct task *prev;             // Previous task in list

    // For Round-Robin scheduling
    uint32_t time_slice;           // Remaining time slice
    uint32_t priority;             // Task priority (reserved for future use)
} task_t;

// Kernel stack size (8KB)
#define KERNEL_STACK_SIZE 0x2000

// User stack size (8KB)
#define USER_STACK_SIZE 0x2000

// task list
extern task_t* task_list_head;
extern task_t* task_list_tail;

// current running task
extern task_t* current;

// Task array (for indexed access, like Linux 0.11)
extern task_t* task[NR_TASKS];

task_t * task_create(void (*entry)(void), uint32_t is_kernel_task);
task_t * task_destroy(task_t *task);
void task_set_state(task_t *task, task_state_t state);
task_state_t task_get_state(task_t *task);
void switch_task(task_t * task);

// Get TSS selector for a task (used by interrupt handler)
uint32_t task_get_tss_selector(task_t *task);

#endif // TASK_H

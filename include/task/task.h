#ifndef TASK_H
#define TASK_H

#include "types.h"

// Task states
typedef enum task_state {
    TASK_READY,      // Ready to run, waiting for CPU
    TASK_RUNNING,    // Currently running on CPU
    TASK_BLOCKED,    // Blocked waiting for I/O or resource
    TASK_ZOMBIE,     // Task finished, waiting to be reaped
    TASK_EXITED      // Task has been cleaned up
} task_state_t;

// Register context for task switching
typedef struct task_context {
    uint32_t edi;
    uint32_t esi;
    uint32_t ebp;
    uint32_t esp;
    uint32_t ebx;
    uint32_t edx;
    uint32_t ecx;
    uint32_t eax;
    uint32_t eip;
    uint32_t eflags;
} task_context_t;

// Process Control Block (PCB)
typedef struct task {
    uint32_t pid;                  // Process ID
    task_state_t state;            // Task state
    uint32_t kernel_stack;         // Kernel stack top address
    uint32_t kernel_stack_size;    // Kernel stack size (in bytes)
    task_context_t context;        // Saved register context

    // For user tasks
    uint32_t user_stack;           // User stack top address
    uint32_t user_stack_size;      // User stack size (in bytes)

    // Task entry point
    void (*entry)(void);           // Task entry function

    // For linked list management
    struct task *next;             // Next task in list
    struct task *prev;             // Previous task in list
} task_t;

// Kernel stack size (8KB)
#define KERNEL_STACK_SIZE 0x2000

// User stack size (8KB)
#define USER_STACK_SIZE 0x2000

#endif // TASK_H

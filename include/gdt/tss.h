#ifndef TSS_H
#define TSS_H

#include "types.h"

// TSS segment selector in GDT
#define TSS_SELECTOR 0x28

// Initialize TSS with kernel stack pointer (for single TSS mode)
void tss_init(uint32_t kernel_stack);

// Update TSS kernel stack (ss0 and esp0) - for single TSS mode
// NOTE: With hardware task switching, each task has its own TSS,
// so this function is only used for the single TSS privilege transition mode.
void tss_set_kernel_stack(uint32_t ss0, uint32_t esp0);

// Get TSS kernel stack pointer
uint32_t tss_get_kernel_stack_esp0(void);

// Get TSS kernel stack segment
uint32_t tss_get_kernel_stack_ss0(void);

#endif // TSS_H

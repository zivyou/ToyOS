#ifndef MULTIBOOT_H
#define MULTIBOOT_H

#include "types.h"

// Multiboot header flags
#define MULTIBOOT_FLAG_MEMORY       0x01
#define MULTIBOOT_FLAG_BOOT_DEVICE  0x02
#define MULTIBOOT_FLAG_CMDLINE      0x04
#define MULTIBOOT_FLAG_MODULES      0x08
#define MULTIBOOT_FLAG_MMAP         0x40

// Multiboot magic number
#define MULTIBOOT_MAGIC             0x2BADB002

// Multiboot info structure
typedef struct multiboot_info {
    uint32_t flags;
    uint32_t mem_lower;
    uint32_t mem_upper;
    uint32_t boot_device;
    uint32_t cmdline;
    uint32_t mods_count;
    uint32_t mods_addr;
    uint32_t syms[4];
    uint32_t mmap_length;
    uint32_t mmap_addr;
} __attribute__((packed)) multiboot_info_t;

// Memory map entry structure
typedef struct multiboot_mmap_entry {
    uint32_t size;
    uint32_t addr_low;
    uint32_t addr_high;
    uint32_t len_low;
    uint32_t len_high;
    uint32_t type;
} __attribute__((packed)) multiboot_mmap_entry_t;

// Memory types
#define MULTIBOOT_MEMORY_AVAILABLE       1
#define MULTIBOOT_MEMORY_RESERVED        2
#define MULTIBOOT_MEMORY_ACPI_RECLAIMABLE  3
#define MULTIBOOT_MEMORY_NVS             4
#define MULTIBOOT_MEMORY_UNUSABLE        5

// Function prototypes
void multiboot_init(uint32_t magic, multiboot_info_t* info);

#endif // MULTIBOOT_H

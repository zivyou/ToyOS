#include "multiboot.h"
#include "printk.h"

uint32_t g_multiboot_magic = 0;
multiboot_info_t* g_multiboot_info = 0;

void parse_memory_map(multiboot_info_t* info) {
    printk("Parsing memory map...\n");

    multiboot_mmap_entry_t* mmap = (multiboot_mmap_entry_t*)info->mmap_addr;
    int count = 0;

    while ((uint32_t)mmap < info->mmap_addr + info->mmap_length) {
        const char* type_str;

        switch(mmap->type) {
            case 1: type_str = "Available"; break;
            case 2: type_str = "Reserved"; break;
            case 3: type_str = "ACPI Reclaimable"; break;
            case 4: type_str = "NVS"; break;
            case 5: type_str = "Unusable"; break;
            default: type_str = "Unknown"; break;
        }

        uint32_t end_addr_low = mmap->addr_low + mmap->len_low;
        uint32_t end_addr_high = mmap->addr_high + mmap->len_high;

        printk("  Region %d: 0x%08x%08x - 0x%08x%08x (0x%08x%08x bytes) %s\n",
               count++,
               mmap->addr_high, mmap->addr_low,
               end_addr_high, end_addr_low,
               mmap->len_high, mmap->len_low,
               type_str);

        // Move to next entry
        mmap = (multiboot_mmap_entry_t*)((uint32_t)mmap + mmap->size + sizeof(mmap->size));
    }

    printk("Total %d memory regions found\n", count);
}

void multiboot_init(uint32_t magic, multiboot_info_t* info) {
    g_multiboot_magic = magic;
    g_multiboot_info = info;

    // Verify magic number
    if (magic != MULTIBOOT_MAGIC) {
        printk("ERROR: Invalid multiboot magic: 0x%x\n", magic);
        return;
    }

    printk("Multiboot info at: 0x%x, flags: 0x%x\n", info, info->flags);

    // Print basic memory info
    if (info->flags & MULTIBOOT_FLAG_MEMORY) {
        printk("Memory: lower=%dKB, upper=%dKB\n",
               info->mem_lower, info->mem_upper);
    }

    // Parse memory map
    if (info->flags & MULTIBOOT_FLAG_MMAP) {
        parse_memory_map(info);
    }
}

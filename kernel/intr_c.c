#include <printk.h>
#include "types.h"
#include "common.h"

void stack_dump(uint32_t err_code, uint32_t addr){
    printk("here");
    hlt();
    uint32_t * esp_ptr = (uint32_t *)(addr+8);
    uint32_t eip = *(esp_ptr);
    uint32_t es = *(esp_ptr+1);
    uint32_t eflags = *(esp_ptr+2);

    uint32_t eax = *(esp_ptr-1);
    uint32_t ebx = *(esp_ptr-2);
    uint32_t ecx = *(esp_ptr-3);
    uint32_t edx = *(esp_ptr-4);

    printk("int-%d, eip=%d, es=%d, eflags=%d, eax=%d, ebx=%d, ecx=%d, edx=%d\n",
        err_code, eip, es, eflags, eax, ebx, ecx, edx);
}
//
// Created by ziv on 2022/3/31.
//
#include "mm/mm.h"
#include "printk.h"

/**
 * 调用BIOS提供的功能，探测一下我们的计算机总共有多少物理内存。
 * 有一点非常蛋疼：要想探测物理内存的情况，不可避免要借助BIOS的功能。而BIOS的功能要想调用，必须要在实模式下，而我们执行到这里的时候，已经是保护模式了，没法再调用BIOS的功能了。
 * 好一点的做法是我们在进入保护模式之前，就将机器的硬件的情况探清楚，然后存储在一个地方（用汇编在main执行前就做好）。然后我们执行到这里的时候，想办法读取一下当时存储的内容就行了。
 * 随着计算机体系结构的发展，为了解决这个麻烦的问题，现在的grub系统会帮我们在启动之前将物理内存的分布探测出来，并且以一定的结构按照约定传递给我们。
 * 具体的描述硬件信息，grub使用的是下面的数据结构。传递的方法是在调用的我们的代码的第一行，将这个数据结构的地址存储在ebx寄存器中，这也是我们为什么需要在header.S中写的
 * 第一行代码是就是movl %ebx, (HADRWARE_INFO)的原因。
*/

// grub设定的描述机器信息的数据结构
typedef
struct hw_info {
        uint32_t flags;                 // Multiboot 的版本信息
        /** 
         * 从 BIOS 获知的可用内存
         *
         * mem_lower和mem_upper分别指出了低端和高端内存的大小，单位是K。
         * 低端内存的首地址是0，高端内存的首地址是1M。
         * 低端内存的最大可能值是640K。
         * 高端内存的最大可能值是最大值减去1M。但并不保证是这个值。
         */
        uint32_t mem_lower;
        uint32_t mem_upper;

        uint32_t boot_device;           // 指出引导程序从哪个BIOS磁盘设备载入的OS映像
        uint32_t cmdline;               // 内核命令行
        uint32_t mods_count;            // boot 模块列表
        uint32_t mods_addr;
        
        /**
         * ELF 格式内核映像的section头表。
         * 包括每项的大小、一共有几项以及作为名字索引的字符串表。
         */
        uint32_t num;
        uint32_t size;
        uint32_t addr;
        uint32_t shndx;

        /**
         * 以下两项指出保存由BIOS提供的内存分布的缓冲区的地址和长度
         * mmap_addr是缓冲区的地址，mmap_length是缓冲区的总大小
         * 缓冲区由一个或者多个下面的大小/结构对 mmap_entry_t 组成
         */
        uint32_t mmap_length;           
        uint32_t mmap_addr;
        
        uint32_t drives_length;         // 指出第一个驱动器结构的物理地址       
        uint32_t drives_addr;           // 指出第一个驱动器这个结构的大小
        uint32_t config_table;          // ROM 配置表
        uint32_t boot_loader_name;      // boot loader 的名字
        uint32_t apm_table;             // APM 表
        uint32_t vbe_control_info;
        uint32_t vbe_mode_info;
        uint32_t vbe_mode;
        uint32_t vbe_interface_seg;
        uint32_t vbe_interface_off;
        uint32_t vbe_interface_len;
} __attribute__((packed)) hw_info_t;


typedef struct mem_map_entry {
    uint32_t size;
    uint32_t* base_addr_lowbits;
    uint32_t* base_addr_highbits;
    uint32_t length_lowbits;
    uint32_t length_highbits;
    uint32_t type;
} __attribute__((packed)) mem_map_entry_t;

extern void* HADRWARE_INFO;
/**
 * 在链接脚本setup.ld中定义了内核使用的地址范围
*/
extern void* _kernel_start;
extern void* _kernel_end;


typedef struct phy_mem_t {
    void* mem_pages[MAX_PAGE_AMOUNT];
    uint32_t used_page_amount;
    uint32_t mem_stack_top;
} __attribute__((packed)) phy_mem_t;

static phy_mem_t phy_mem = {
    .used_page_amount = 0,
    .mem_stack_top = 0,
};

/**
 * 接下来我们遇到第二个问题，已知grub将物理内存布局通过ebx传递给了我们，我们也将其存储在HADRWARE_INFO中，我们要怎么样在保护模式下读取这个值呢？
 * 有一种简单的做法，就是将整个内核的.text/.data/.bss等这样的区域，直接映射到3G-4G的地方，这样即便我们在保护模式下，直接读就行。
 * 所以我们写下面这个方法，在启动保护模式之前就动手映射一下。
 * 由于要在实模式下调用这个方法，所以必须要将这个代码放在.init.text区域
*/
__attribute__((section(".init.text"))) void init_paging() {
    /* 
    * 在我们自己虚拟内存管理之前，我们的物理内存布局是由BIOS系统给我们规划的。现代的BIOS规划的物理内存布局一般是这样的：
    * 可以看得到总共是1MB的物理内存空间.
    * 我们在链接脚本setup.ld中，将内核代码的起始地址设置在了0x100000处，这是BIOS没有规定的保留区域。
    */
   /*
+------------------------------------------+ 0xFFFFF
|                                          |
|                                          |
|                                          |           64KB
|       BIOS Routine                       |
+------------------------------------------+ 0xF0000
|                                          |
|      ROM/Mappered IO                     |           160KB
+------------------------------------------+ 0xC8000
|      Graphic Adapter BIOS                |           32KB
+------------------------------------------+ 0xB8000
|                                          |           32KB
|       Mono Text Video Buffer             |
+------------------------------------------+ 0xB0000
|      Graphic Video Buffer                |           64KB
+------------------------------------------+ 0xA0000
|      Extended BIOS Data Area             |           1KB
+------------------------------------------+ 0x9FC00
|                                          |
|                                          |
|                                          |           607KB+512B
|                                          |
|                                          |
+------------------------------------------+ 0x07E00
|             Boot Sector                  |           512B
+------------------------------------------+ 0x07C00
|                                          |
|                                          |           29KB+768B
+------------------------------------------+ 0x00500
|           BIOS Data Area                 |           256B
+------------------------------------------+ 0x00400
|      Interrupt Vector Table              |           1KB
+------------------------------------------+ 0x00000
   */

    hw_info_t* hw_info = HADRWARE_INFO;
    dump_physical_mem_map(hw_info);
    init_physical_mem_map(hw_info);
    void* temp_page_directory = 0x1000;
    __asm__ ("movl %0, %%cr3"::"r"(temp_page_directory));
}

__attribute__((section(".init.text"))) void init_physical_mem_map(hw_info_t* hw_info) {
    for (int i=0; i<hw_info->mmap_length; i+=sizeof(mem_map_entry_t)) {
        mem_map_entry_t* entry = hw_info->mmap_addr+i;
        if (entry->type == 1 && entry->base_addr_lowbits == &_kernel_start) {
            // 这片区域是我们自己写的内核所在地，得存起来
            uint32_t kernel_size = &_kernel_end - &_kernel_start;
            uint32_t start = &_kernel_start;
            uint32_t end = &_kernel_end;
            uint32_t entry_end = entry->base_addr_lowbits + entry->length_lowbits;
            while (start < entry_end && start <= MAX_MEM_SIZE) {
                phy_mem.mem_pages[++(phy_mem.mem_stack_top)] = start;
                start += PAGE_SIZE;
                phy_mem.used_page_amount++;
            }
        }
    }
}

__attribute__((section(".init.text"))) void dump_physical_mem_map(hw_info_t* hw_info) {
    uint32_t kernel_start = &_kernel_start; // https://blog.csdn.net/czg13548930186/article/details/78535419
    uint32_t kernel_end = &_kernel_end;
    uint32_t kernel_size = (kernel_end-kernel_start)/1024;
    printk("kenerl address: start=0x%x, end=0x%x, size=%dkB\n", kernel_start, kernel_end, kernel_size);
    printk("physical memory map: .......start at: 0x%x, entry amounts: %d\n", hw_info->mmap_addr, hw_info->mmap_length);
    for (int i=0; i<hw_info->mmap_length; i+=sizeof(mem_map_entry_t)) {
        mem_map_entry_t* entry = hw_info->mmap_addr+i;
        uint32_t start = entry->base_addr_lowbits;
        uint32_t end = entry->base_addr_lowbits+entry->length_lowbits;
        printk("index = %d, start = 0x%x, end = 0x%x\n", i, start, end);
    }
}

void* phy_alloc_page() {
    void* page = phy_mem.mem_pages[(phy_mem.mem_stack_top)--];
    phy_mem.used_page_amount--;
    return page;
}

void mm_init() {

}
#include <types.h>

// 共2+2+1+1+1+1=8个字节
typedef struct gdt_entry{
    uint16_t limit_low; // limit: 0-15    // 0-15
    uint16_t base_low; //Base: 0-15       // 16-31
    uint8_t base_mid; //base: 16-23       // 32-39
    uint8_t type:4;   //                  // 40-43
                    //[E, DC, RW, A], E=1代表code segment; E=0代表data segment;
                    // DC指定方向： 对于data segment指定地址从高到低还是从低到高；对于code segment,指定代码在ring0-ring3见互相跳转的优先级鉴定
                    // RW:读写权限
                    // A: 访问权限，最好设置成0；放心，即使设置成0 CPU也能访问的（CPU是超级权限）
    uint8_t    s:1;      //system sign: is this entry a system segment, or code segment/data segment  // 44-44
                         // 例如，一个tss entry是一个系统维护的entry，那么s=0；其他的code segment/data segment,s=1
    uint8_t  dpl:2;    //descriptor privilege level， ring0: dpl=0, ring3: dpl=3     // 45-46
    uint8_t    p:1;        //segment present,对于任何有效的entry,p=1    // 47-47
    // -- granularity begin
    uint8_t limit_high:4; // limit: 16-19               // 48-51
    uint8_t        avl:1; //ignore                      // 52-52
    uint8_t          o:1;   // o=1,说明是64位代码段        // 53-53
    uint8_t          b:1;    // b=0:16位保护模式段，b=1:32位保护模式段               // 54-54
    uint8_t          g:1;   //g=0: segment length=1MB; g=1: segment length=4GB  // 55-55
    // --- granularity end
    uint8_t base_high; //base: 24-31                    // 56-63
   
}__attribute__((packed)) gdt_entry ;

typedef struct gdt_ptr{
    uint16_t limit;
    uint32_t base;
}__attribute__((packed)) gdt_prt ;

static struct gdt_entry gdt[6];
struct gdt_ptr gp;

void set_gdt_entry(int index, uint32_t base, uint32_t limit, uint32_t type, uint32_t s, uint32_t dpl, uint32_t p, uint32_t avl, uint32_t o, uint32_t b, uint32_t g){
    gdt[index].type = type & 0x0F;
    gdt[index].s = s;
    gdt[index].dpl = dpl & 0x03;
    gdt[index].p = p;
    gdt[index].avl = avl;
    gdt[index].o = o;
    gdt[index].b = b;
    gdt[index].g = g;
    gdt[index].limit_low = limit & 0xFFFF;
    gdt[index].limit_high = (limit>>16) & 0x00FF;
    gdt[index].base_low = (base) & 0xFFFF;
    gdt[index].base_mid = (base>>16) & 0x00FF;
    gdt[index].base_high = (base>>24)&0xFF;
}

extern  void gdt_flush(uint32_t);
extern void tss_flush();

typedef struct tss_entry {
    uint32_t  ts_link;  // old ts selector
    uint32_t  ts_esp0;  // stack pointers and segment selectors
    uint32_t  ts_ss0;  // after an increase in privilege level
    uint32_t  ts_esp1;
    uint32_t  ts_ss1;
    uint32_t  ts_esp2;
    uint32_t  ts_ss2;
    uint32_t  ts_cr3;  // page directory base
    uint32_t  ts_eip;  // saved state from last task switch
    uint32_t  ts_eflags;
    uint32_t  ts_eax;  // more saved state (registers)
    uint32_t  ts_ecx;
    uint32_t  ts_edx;
    uint32_t  ts_ebx;
    uint32_t  ts_esp;
    uint32_t  ts_ebp;
    uint32_t  ts_esi;
    uint32_t  ts_edi;
    uint32_t  ts_es;  // even more saved state (segment selectors)
    uint32_t  ts_cs;
    uint32_t  ts_ss;
    uint32_t  ts_ds;
    uint32_t  ts_fs;
    uint32_t  ts_gs;
    uint32_t  ts_ldt;
    uint32_t  ts_t;  // trap on task switch
    uint32_t  ts_iomb;  // i/o map base address

} __attribute__((packed)) tss_entry;

static struct tss_entry tss __attribute__((aligned(8)));

// 各个内存段所在全局描述符表下标
#define SEG_NULL    0
#define SEG_KTEXT   1
#define SEG_KDATA   2
#define SEG_UTEXT   3
#define SEG_UDATA   4
#define SEG_TSS     5
#define GD_KTEXT    ( (SEG_KTEXT) << 3)     // 内核代码段 0x08
#define GD_KDATA    ( (SEG_KDATA) << 3)     // 内核数据段
#define GD_UTEXT    ( (SEG_UTEXT) << 3)     // 用户代码段
#define GD_UDATA    ( (SEG_UDATA) << 3)     // 用户数据段
#define GD_TSS      ( (SEG_TSS) << 3)       // 任务段
// 段描述符 DPL
#define DPL_KERNEL  (0)// 内核级
#define DPL_USER    (3)// 用户级

// 各个段的全局描述符表的选择子
#define KERNEL_CS   ( (GD_KTEXT) | DPL_KERNEL)
#define KERNEL_DS   ( (GD_KDATA) | DPL_KERNEL)
#define USER_CS     ( (GD_UTEXT) | DPL_USER)
#define USER_DS     ( (GD_UDATA) | DPL_USER)

void set_gdt_entry_for_tss(int32_t index, uint32_t base, uint32_t limit, uint32_t access, uint32_t gran) {
    gdt[index].base_low     = (base & 0xFFFF);
    gdt[index].base_mid  = (base >> 16) & 0xFF;
    gdt[index].base_high    = (base >> 24) & 0xFF;

    gdt[index].limit_low    = (limit & 0xFFFF);
    gdt[index].s = (access>>4) & 0x01;
    gdt[index].dpl  = (access>>5) & 0x03;

    gdt[index].p = (access>>7) & 0x01;
    gdt[index].avl       = 0;
    gdt[index].limit_high = (limit >> 28) & 0x0F;
    gdt[index].o    = (gran>>5) & 0x01;
    gdt[index].b    = (gran>>6) & 0x01;
    gdt[index].g    = (gran>>7) & 0x01;
    gdt[index].type = access & 0x0F;
    return;
}



void set_tss_entry(uint32_t index, uint32_t ss0, uint32_t esp0) {
    uint32_t base = (uint32_t)&tss;
    uint32_t limit = base + sizeof(tss);

    set_gdt_entry_for_tss(index, base, limit, 0x89, 0x40);

    tss.ts_ss0 = ss0;
    tss.ts_esp0 = esp0;
    tss.ts_cs = USER_CS;
    tss.ts_ss = USER_DS;
    tss.ts_ds = USER_DS;
    tss.ts_es = USER_DS;
    tss.ts_fs = USER_DS;
    tss.ts_gs = USER_DS;
}

void gdt_init(){
    gp.base = (uint32_t)&gdt;
    gp.limit = (uint16_t)(sizeof(gdt)-1);
    set_gdt_entry(0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
    set_gdt_entry(1, 0, 0xFFFFFFFF, 0x0A, 1, 0, 1, 0, 0, 1, 1); //code segment;
    set_gdt_entry(2, 0, 0xFFFFFFFF, 0x02, 1, 0, 1, 0, 0, 1, 1); //data segment;
    // 用户模式代码段
    set_gdt_entry(3, 0, 0xFFFFFFFF, 0x0A,1,3,1,0,0,1,1);
    // 用户模式数据段
    set_gdt_entry(4, 0, 0xFFFFFFFF, 0x02,1,3,1,0,0,1,1);
    set_tss_entry(5, KERNEL_DS, 0);
    gdt_flush((uint32_t)&gp);
    tss_flush();
}

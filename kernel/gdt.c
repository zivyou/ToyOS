#include <types.h>


typedef struct gdt_entry{
    uint16_t limit_low; // limit: 0-15
    uint16_t base_low; //Base: 0-15
    uint8_t base_mid; //base: 16-23
    uint8_t type:4;   //
    uint8_t    s:1;      //system sign: is this entry a system segment, or code segment/data segment
    uint8_t  dpl:2;    //descriptor privilege level
    uint8_t    p:1;        //segment present
    uint8_t limit_high:4; // limit: 16-19
    uint8_t        avl:1; //ignore
    uint8_t          o:1;
    uint8_t          b:1;    //
    uint8_t          g:1;   //g=0: segment length=1MB; g=1: segment length=4GB
    uint8_t base_high; //base: 24-31
   
}__attribute__((packed)) gdt_entry ;

typedef struct gdt_ptr{
    uint16_t limit;
    uint32_t base;
}__attribute__((packed)) gdt_prt ;

static struct gdt_entry gdt[3];
struct gdt_ptr gp;

void set_gdt_entry(int index, uint32_t base, uint32_t limit, uint8_t type, uint8_t s, uint8_t dpl, uint8_t p, uint8_t avl, uint8_t o, uint8_t b, uint8_t g){
    gdt[index].type = type & 0x0F;
    gdt[index].s = s;
    gdt[index].dpl = dpl & 0x03;
    gdt[index].p = p;
    gdt[index].avl = avl;
    gdt[index].o = o;
    gdt[index].b = b;
    gdt[index].g = g;
    gdt[index].limit_low = limit & 0x0000FFFF;
    gdt[index].limit_high = (limit>>16) & 0x000000FF;
    gdt[index].base_low = (base) & 0x0000FFFF;
    gdt[index].base_mid = (base>>16) & 0x000000FF;
    gdt[index].base_high = (base>>24)&0x000000FF;
}


extern  void gdt_flush(uint32_t);
void gdt_init(){
    gp.base = &gdt;
    gp.limit = sizeof(gdt)-1;
    set_gdt_entry(0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
    set_gdt_entry(1, 0, 0xFFFFFFFF, 0x0A, 1, 0, 1, 0, 0, 1, 1); //code segment;
    set_gdt_entry(2, 0, 0xFFFFFFFF, 0x01, 1, 0, 1, 0, 0, 1, 1); //data segment;
    
    gdt_flush((uint32_t)&gp);
}

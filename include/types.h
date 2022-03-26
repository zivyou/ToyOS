#ifndef TYPES_H
#define TYPES_H

typedef unsigned char   uint8_t;
typedef unsigned short  uint16_t;
typedef unsigned int    uint32_t;
typedef unsigned long long      uint64_t;

typedef char    int8_t;
typedef short   int16_t;
typedef int     int32_t;
typedef long long       int64_t;

typedef unsigned int size_t;

typedef struct registers_ptr_t {
    uint16_t ds;
    uint16_t padding1;

    uint32_t edi;
    uint32_t esi;
    uint32_t ebp;
    uint32_t oesp;
    uint32_t ebx;
    uint32_t edx;
    uint32_t ecx;
    uint32_t eax;

    // 这个是我们自己在调用common_isr之前调用的
    uint32_t int_no;

    // 这个有一些中断会压，有一些不压
    uint32_t err_codes;

    // 下面是CPU自动压栈的
    uint32_t eip;
    uint16_t cs;
    uint16_t padding2;
    uint32_t eflags;
    uint32_t esp;
    uint16_t ss;
    uint16_t padding3;
} registers_ptr_t;

#endif
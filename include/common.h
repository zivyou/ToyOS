#ifndef COMMON_H
#define COMMON_H
#include <types.h>

void outb(uint16_t addr, uint8_t data);
uint8_t inb(uint16_t addr);
void hlt(void);
void enable_interrput();
void disable_interrupt();
#endif
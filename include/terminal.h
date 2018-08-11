#ifndef TERMINAL_H
#define TERMINAL_H
#include "types.h"

void terminal_init();
void terminal_print(char *str);
void terminal_scroll(int l);

#endif
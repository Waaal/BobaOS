#ifndef TERMINAL_H
#define TERMINAL_H

#include <stdint.h>

#define VGA_WIDTH 80
#define VGA_HEIGHT 25

#define TERMINAL_BACKCOLOR 0x5
#define TERMINAL_FORECOLOR 0xF

void terminalPrint(const char* c);
void terminalInit();

//void kprintf(const char* string, ...);

#endif

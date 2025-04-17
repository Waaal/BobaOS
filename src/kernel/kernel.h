#ifndef KERNEL_H
#define KERNEL_H

#include <stdint.h>

#define VGA_WIDTH 80
#define VGA_HEIGHT 25

#define TERMINAL_BACKCOLOR 0x5
#define TERMINAL_FORECOLOR 0xF

uint8_t curRow = 0;
uint8_t curCol = 0;

uint16_t* videoMemory = (uint16_t*)0xb8000;

void kmain();

#endif

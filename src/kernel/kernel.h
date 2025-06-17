#ifndef KERNEL_H
#define KERNEL_H

#include "idt/idt.h"
#include "memory/paging/paging.h"

enum panicType
{
	PANIC_TYPE_KERNEL = 0,
	PANIC_TYPE_EXCEPTION = 1
}; 

void kmain();
void panic(enum panicType type, struct trapFrame* frame, const char* message);
PML4Table getKernelPageTable();

#endif

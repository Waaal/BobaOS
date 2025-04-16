#include "kernel.h"

#include "memory/kheap/kheap.h"
#include "memory/memory.h"
#include "gdt/gdt.h"
#include "memory/paging/paging.h"

uint8_t terminalNextCharSpecial = 0;

void terminal_init()
{
	for(uint16_t i = 0; i < VGA_WIDTH * VGA_HEIGHT; i++)
	{
		videoMemory[i] = (TERMINAL_BACKCOLOR << 12) + (TERMINAL_FORECOLOR << 8);
	}
}

void terminal_moveUp()
{
	uint64_t moveBytes = (VGA_WIDTH * VGA_HEIGHT * 2) - (VGA_WIDTH*2);
	void* src = (void*)(videoMemory + (VGA_WIDTH));

	memcpy(videoMemory, src ,moveBytes);
	
	//clear all letters
	for(uint8_t i = 0; i < VGA_WIDTH; i++)
	{
		uint64_t test = (VGA_WIDTH * (VGA_HEIGHT - 1)) + i;
		videoMemory[test] &= 0xFF00;
	}

	curRow--;
}

void terminal_putchar(char c)
{

	if(curCol == VGA_WIDTH)
	{
		curRow++;
		curCol = 0;
	}

	if(curRow == VGA_HEIGHT)
	{
		terminal_moveUp();
	}
	
	if(c == '/')
	{
		terminalNextCharSpecial = 1;
		return;
	}

	if(terminalNextCharSpecial == 1)
	{
		terminalNextCharSpecial = 0;
		if(c == 'n')
		{
			curRow++;
			curCol = 0;

			return;
		}
	}

	videoMemory[curCol + curRow*VGA_WIDTH] += c;
	curCol++;
}

void terminal_print(const char* str)
{
	while(*str != 0)
	{
		terminal_putchar(*str++);
	}
}

struct gdt gdt[] = {
	{ .limit = 0x0, .base = 0x0, .access = 0x0,  .flags = 0x0 }, // NULL
	{ .limit = 0x0, .base = 0x0, .access = 0x98, .flags = 0x2 }, //	K CODE
	{ .limit = 0x0, .base = 0x0, .access = 0x90, .flags = 0x2 }  // K DATA
};

void kmain()
{
	loadGdt(gdt);
	
	terminal_init();
	terminal_print("Hello World/n");
	
	kheap_init();	

	PLM4Table kernelPageTable = createKernelTable(0x0, 0x0, 0x100000000);
	
	uint8_t* test1 = kzalloc(1);
	uint8_t* test2 = kzalloc(1);

	*test1 = 100;
	*test2 = 200;

	remapPage(test1, test2, kernelPageTable);
	*test1 = 5;

	while(1){}
}

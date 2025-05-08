#include "terminal.h"

#include <stddef.h>

#include "memory/kheap/kheap.h"
#include "memory/memory.h"
#include "string/string.h"
#include "koal/koal.h"

int terminalPutchar(const char c);
int terminalPrint(const char* str);

struct kernelOutputAbstractionLayer koal = 
{
	.outChar = terminalPutchar,
	.outString = terminalPrint
};

uint8_t curRow = 0;
uint8_t curCol = 0;

uint16_t* videoMemory = (uint16_t*)0xb8000;

void terminalClear(uint8_t color)
{
	for(uint16_t i = 0; i < VGA_WIDTH * VGA_HEIGHT; i++)
	{
		videoMemory[i] = (color << 12) + (TERMINAL_FORECOLOR << 8);
	}
	curRow = 0;
	curCol = 0;
}

void terminalInit()
{
	strncpy(koal.name, "TEXT_TERMINAL", 32);
	koalAttach(&koal);

	for(uint16_t i = 0; i < VGA_WIDTH * VGA_HEIGHT; i++)
	{
		videoMemory[i] = (TERMINAL_BACKCOLOR << 12) + (TERMINAL_FORECOLOR << 8);
	}
}

static void terminalMoveUp()
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

int terminalPutchar(char c)
{
	if(curCol == VGA_WIDTH)
	{
		curRow++;
		curCol = 0;
	}

	if(curRow == VGA_HEIGHT)
	{
		terminalMoveUp();
	}
	
	if(c == 0xA) // \n
	{
		curRow++;
		curCol = 0;
		return 0;
	}

	videoMemory[curCol + curRow*VGA_WIDTH] += c;
	curCol++;

	return 0;
}

int terminalPrint(const char* str)
{
	while(*str != 0)
	{
		terminalPutchar(*str++);
	}
	return 0;
}

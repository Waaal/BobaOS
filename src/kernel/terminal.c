#include "terminal.h"

#include "memory/memory.h"

uint8_t terminalNextCharSpecial = 0;

void terminalInit()
{
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

static void terminalPutchar(char c)
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

void terminalPrint(const char* str)
{
	while(*str != 0)
	{
		terminalPutchar(*str++);
	}
}

#include "terminal.h"

#include "memory/kheap/kheap.h"
#include "memory/memory.h"
#include "string/string.h"

uint8_t curRow = 0;
uint8_t curCol = 0;

uint16_t* videoMemory = (uint16_t*)0xb8000;

uint8_t terminalNextCharSpecial = 0;

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

static char* strToUInt(uint64_t num)
{
	char* numberMap = "0123456789";
	char* ret = (char*)kzalloc(21);
	
	uint8_t i = 0;
	for(i = 0; i < 20; i++)
	{
		uint8_t number = num % 10;
		char c = numberMap[number];
		ret[19 - i] = c;
		
		num /= 10;
		if(num == 0)
		{
			break;
		}
	}

	memcpy(ret, ret+(19-i), i+1);
	ret[i+1] = 0x0;
	
	return ret;
}

static char* strToHex(uint64_t num)
{
	char* numberMap = "0123456789ABCDEF";
	char* ret = (char*)kzalloc(23);
	ret[0] = '0';
	ret[1] = 'x';

	uint8_t i = 0;
	for(i = 2; i < 22; i++)
	{
		uint8_t number = num % 16;
		char c = numberMap[number];
		ret[22 - i] = c;
		
		num /= 16;
		if(num == 0)
		{
			break;
		}
	}

	memcpy(ret+2, ret+(22-i), i+1);
	ret[i+1] = 0x0;
	
	return ret;
}

void kprintf(const char* string, ...)
{
	va_list args;
	va_start(args, string);
	
	uint8_t specialChar = 0;
	for(uint64_t i = 0; i < strlen(string); i++)
	{
		char c = string[i];
		if(specialChar)
		{
			switch(c)
			{
				case 'x':
				{
					char* hexChar = strToHex(va_arg(args, uint64_t));
					terminalPrint(hexChar);

					kzfree(hexChar);
					break;
				}
				case 'u':
				{
					char* uIntChar = strToUInt(va_arg(args, uint64_t));
					terminalPrint(uIntChar);

					kzfree(uIntChar);
					break;
				}
				case 's':
					terminalPrint(va_arg(args, char*));
					break;
				default:
					break;
			}
			specialChar = 0;
			continue;
		}

		if(c == '%')
		{
			specialChar = 1;
			continue;
		}

		terminalPutchar(c);
	}

	va_end(args);
}


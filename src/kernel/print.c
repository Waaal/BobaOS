#include "print.h"

#include <stdint.h>
#include <stddef.h>

#include "memory/kheap/kheap.h"
#include "memory/memory.h"
#include "string/string.h"
#include "koal/koal.h"

static void strToUInt(uint64_t num, char* oRes)
{
	char* numberMap = "0123456789";

	uint8_t i = 0;
	for(i = 0; i < 20; i++)
	{
		uint8_t number = num % 10;
		char c = numberMap[number];
		oRes[19 - i] = c;
		
		num /= 10;
		if(num == 0)
		{
			break;
		}
	}

	memcpy(oRes, oRes+(19-i), i+1);
	oRes[i+1] = 0x0;	
}

static void strToHex(uint64_t num, char* oRes)
{
	char* numberMap = "0123456789ABCDEF";

	oRes[0] = '0';
	oRes[1] = 'x';

	uint8_t i = 0;
	for(i = 2; i < 22; i++)
	{
		uint8_t number = num % 16;
		char c = numberMap[number];
		oRes[22 - i] = c;
		
		num /= 16;
		if(num == 0)
		{
			break;
		}
	}

	memcpy(oRes+2, oRes+(22-i), i+1);
	oRes[i+1] = 0x0;
}

void kprintf(const char* str, ...)
{
	va_list args;
	va_start(args, str);
	
	uint8_t specialChar = 0;
	for(uint64_t i = 0; i < strlen(str); i++)
	{
		char c = str[i];
		if(specialChar)
		{
			switch(c)
			{
				case 'x':
				{
					char hexChar[32];
					strToHex(va_arg(args, uint64_t), hexChar);
					koalPrint(hexChar);
					break;
				}
				case 'u':
				{
					char uIntChar[32];
					strToUInt(va_arg(args, uint64_t), uIntChar);
					koalPrint(uIntChar);
					break;
				}
				case 's':
					koalPrint(va_arg(args, char*));
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

		koalPrintChar(c);
	}

	va_end(args);
}

void print(const char* str)
{
	koalPrint(str);
}

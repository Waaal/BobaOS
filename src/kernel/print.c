#include "print.h"

#include <stdint.h>
#include <stddef.h>

#include "memory/kheap/kheap.h"
#include "memory/memory.h"
#include "string/string.h"
#include "koal/koal.h"

static char* strToUInt(uint64_t num)
{
	char* numberMap = "0123456789";
	char* ret = (char*)kzalloc(21);
	
	if(ret == NULL){return NULL;}

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
	
	if(ret == NULL){return NULL;}

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
					char* hexChar = strToHex(va_arg(args, uint64_t));
					if(hexChar == NULL){break;}
					koalPrint(hexChar);

					kzfree(hexChar);
					break;
				}
				case 'u':
				{
					char* uIntChar = strToUInt(va_arg(args, uint64_t));
					if(uIntChar == NULL){break;}
					koalPrint(uIntChar);

					kzfree(uIntChar);
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

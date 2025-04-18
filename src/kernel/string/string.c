#include "string.h"

uint64_t strlen(const char* str)
{
	uint64_t len = 0;
	while(*str++ != 0x0)
	{
		len++;
	}
	return len;
}

int8_t strcmp(const char* str1, const char* str2)
{
	int8_t ret = 0;
	while(1)
	{
		if(*str1 == 0x0 && *str1 == 0x0)
		{
			break;
		}

		if(*str1 != *str2)
		{
			ret = *str1 > *str2 ? 1 : -1;
			break;
		}

		str1++;
		str2++;
	}

	return ret;
}


void strcpy(char* dest, const char* src)
{
	while(*dest != 0x0 && *src != 0x0)
	{
		*dest = *src;
		dest++;
		src++;
	}
}

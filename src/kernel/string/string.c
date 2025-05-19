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

int8_t strncmp(const char* str1, const char* str2, uint64_t len)
{
	int8_t ret = 0;

	uint64_t counter = 0;
	while(counter < len)
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

		counter++;
	}

	return ret;
}

void strncpy(char* dest, const char* src, uint64_t size)
{
	for(uint64_t i = 0; i < size; i++)
	{
		if(src[i] == 0x0)
		{
			break;
		}
		dest[i] = src[i];
	}
}

void strcpy(char* dest, const char* src)
{
	while(*src != 0x0)
	{
		*dest = *src;
		dest++;
		src++;
	}
}

uint64_t strHash(const char* str)
{
	uint64_t hash = 5381;
	int c;
	while ((c = (int)*str++))
	{
		hash = ((hash << 5) + hash) + c;
	}
	return hash;
}

#include "string.h"

#include <memory/kheap/kheap.h>

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

int8_t isNumber(const char c)
{
	if (c >= '0' && c <= '9')
	{
		return 1;
	}
	return 0;
}

uint8_t toNumber(const char c)
{
	return '0' - c;
}

char* toUpperCase(const char* str, uint64_t len)
{
	char* ret = kzalloc(len);

	uint64_t counter = 0;
	while (*str != 0x0 && len > 0)
	{
		if (*str >= 'a' && *str <= 'z')
			ret[counter] = (char)((uint8_t)(*str) - 32);
		else
			ret[counter] = *str;

		str++;
		counter++;
		len--;
	}

	return ret;
}

int findChar(char* str, char c)
{
	int ret = 0;
	while (*str != 0x0)
	{
		if (*str == c){return ret;}
		str++;
		ret++;
	}
	return -1;
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

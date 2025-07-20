#include "string.h"

#include "memory/kheap/kheap.h"

uint64_t strlen(const char* str)
{
	uint64_t len = 0;
	while(*str++ != 0x0)
	{
		len++;
	}
	return len;
}

int strcmp(const char* str1, const char* str2)
{
	int ret = 0;
	while(1)
	{
		if(*str1 == 0x0 && *str2 == 0x0)
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

int strncmp(const char* str1, const char* str2, uint64_t len)
{
	int ret = 0;

	uint64_t counter = 0;
	while(counter < len)
	{
		if(*str1 == 0x0 && *str2 == 0x0)
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
	uint64_t i = 0;
	for(i = 0; i < size; i++)
	{
		if(src[i] == 0x0) break;
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
	*dest = 0x0;
}

//Copys max bytes. Is null terminated
void strcpym(char* dest, const char* src, uint64_t max)
{
	uint64_t i = 0;
	for(i = 0; i < max; i++)
	{
		if(src[i] == 0x0)
		{
			break;
		}
		dest[i] = src[i];
	}
	dest[i] = 0x00;
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
	ret[counter] = 0x0;

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

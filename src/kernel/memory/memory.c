#include "memory.h"

//Sets each byte (length of size) to value
void memset(void* addr, uint8_t value, uint64_t size)
{
	for(uint64_t i = 0; i < size; i++)
	{
		((uint8_t*)addr)[i] = value;
	}
}

//Copy size bytes from src to dest
void memcpy(void* dest, void* src, uint64_t size)
{
	for(uint64_t i = 0; i < size; i++)
	{
		((uint8_t*)dest)[i] = ((uint8_t*)src)[i];
	}
}

//Compares each byte from ptr1 with ptr2 with the length of size
//Returns 
//  0: ptr1 == ptr2
// -1: ptr1 < ptr2
//  1: ptr1 > ptr2
int8_t memcmp(void* ptr1, void* ptr2, uint64_t size)
{
	for(uint64_t i = 0; i < size; i++)
	{
		if(((uint8_t*)ptr1)[i] != ((uint8_t*)ptr2)[i])
		{
			return (((uint8_t*)ptr1)[i] < ((uint8_t*)ptr2)[i]) ? -1 : 1;	
		}

	}
	return 0;
}

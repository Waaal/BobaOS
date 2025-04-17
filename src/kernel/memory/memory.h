#ifndef MEMORY_H
#define MEMORY_H

#include <stdint.h>

void memset(void* addr, uint8_t value, uint64_t size);
void memcpy(void* dest, void* src, uint64_t size);
int8_t memcmp(void* ptr1, void* ptr2, uint64_t size);


#endif

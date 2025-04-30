#ifndef KHEAP_H
#define KHEAP_H

#include <stdint.h>

enum kernelHeapType
{
	KERNEL_HEAP_TYPE_PAGE,
	KERNEL_HEAP_TYPE_BUDDY
};

int kheapInit(enum kernelHeapType type);

void* kzalloc(uint64_t size);
int kzfree(void* pointer);

#endif

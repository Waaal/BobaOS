#include "kheap.h"

#include "status.h"
#include "kheap_page.h"
#include "kheap_buddy.h"

enum kernelHeapType selected;

int kheapInit(enum kernelHeapType type)
{
	int ret = 0;
	switch(type)
	{
		case KERNEL_HEAP_TYPE_PAGE:
			ret = kheapInitPage();
			break;
		case KERNEL_HEAP_TYPE_BUDDY:
			ret = kheapInitBuddy();
			break;
		default: return -EIARG;
	}

	selected = type;
	return ret;
} 

void* kzalloc(uint64_t size)
{
	void* ret;
	if(selected == KERNEL_HEAP_TYPE_PAGE)
	{
		ret = kzallocPage(size);
	}
	else
	{
		ret = kzallocBuddy(size);
	}

	return ret;
}

int kzfree(void* pointer)
{
	int ret;
	if(selected == KERNEL_HEAP_TYPE_PAGE)
	{
		ret = kzfreePage(pointer);
	}
	else
	{
		ret = kzfreeBuddy(pointer);
	}

	return ret;
}

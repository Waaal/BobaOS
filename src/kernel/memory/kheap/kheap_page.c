#include "kheap_page.h"

#include <stddef.h>

#include "config.h"
#include "memory/memory.h"
#include "status.h"
#include "memory/paging/paging.h"
#include "print.h"

uint64_t table_size = -1;
uint64_t tableMaxSize = -1;

static int kheapInitTable()
{
	table_size = BOBAOS_KERNEL_HEAP_SIZE / KHEAP_BLOCK_SIZE;
	tableMaxSize = getMaxMemorySize() - getUpperMemorySize();

	if((table_size + BOBAOS_KERNEL_HEAP_TABLE_ADDRESS) > tableMaxSize)
	{
		return -ENMEM;
	}
	kprintf("Kernel heap table\n  Max size: %x Actual size: %x Start: %x\n\n", tableMaxSize, table_size, BOBAOS_KERNEL_HEAP_TABLE_ADDRESS);	
	
	memset((void*)BOBAOS_KERNEL_HEAP_TABLE_ADDRESS, 0x0, table_size);
	return 0;
}

static int checkAlignment(uint64_t page)
{
	return page % KHEAP_BLOCK_SIZE == 0 ? 0 : -1;
}

static uint64_t sizeUpToPage(uint64_t size)
{
	if(checkAlignment(size) == 0)
	{
		return size;
	} 
	return size + (KHEAP_BLOCK_SIZE - (size % KHEAP_BLOCK_SIZE));
}

static int getPageCount(uint64_t size, uint64_t* pagesOut)
{
	int ret = 0;
	if(size % KHEAP_BLOCK_SIZE != 0)
	{
		ret = -EIO;
		goto out;
	}
	
	*pagesOut = size / KHEAP_BLOCK_SIZE;
out:
	return ret;
}

static KHEAP_TABLE_ENTRY getTableEntry(uint64_t offset)
{
	return ((KHEAP_TABLE_ENTRY)(BOBAOS_KERNEL_HEAP_TABLE_ADDRESS + offset));
}

static void markEntryTaken(uint64_t startIndex, uint64_t size)
{
	for(uint64_t i = 0; i < size; i++)
	{
		KHEAP_TABLE_ENTRY tableEntry = getTableEntry(i + startIndex); 		
		
		if(i == 0)
		{
			*tableEntry |= KHEAP_FLAG_START;
		}

		if(i+1 != size)
		{
				*tableEntry |= KHEAP_FLAG_NEXT;
		}
			*tableEntry |= KHEAP_FLAG_LOCKED;
	}
}

static int findFreePages(uint64_t pagesCount, void** memOut)
{
	uint64_t foundCount = 0; 
	for(uint64_t i = 0; i < table_size; i++)
	{
		KHEAP_TABLE_ENTRY tableEntry = getTableEntry(i);

		if((*tableEntry & KHEAP_FLAG_LOCKED) == 0)
		{
			foundCount++;
		}
		else
		{
			foundCount = 0;
		}

		if(foundCount == pagesCount)
		{
			markEntryTaken(i+1 - foundCount, pagesCount);
			*memOut = (void*)(BOBAOS_KERNEL_HEAP_ADDRESS + ((i+1 - foundCount) * KHEAP_BLOCK_SIZE));
			return 0;
		}
	}
	return -ENMEM;
}

//Allocates memory on the kernel stack. If returned address == 0x0, then there is a -ENMEM error
void* kzallocPage(uint64_t size)
{	
	void* mem = NULL;
	uint64_t pagesCount = 0;

	size = sizeUpToPage(size);

	getPageCount(size, &pagesCount);
	if(findFreePages(pagesCount, &mem) < 0)
	{
		goto out;
	}
	
	memset(mem, 0x0, pagesCount * KHEAP_BLOCK_SIZE);
out:
	return mem;
} 

int addressToTableIndex(void* address)
{
	if((uint64_t)address < BOBAOS_KERNEL_HEAP_ADDRESS && (uint64_t)address > tableMaxSize)
	{
		return -EIARG;
	}
	return (((uint64_t)address) - BOBAOS_KERNEL_HEAP_ADDRESS) / KHEAP_BLOCK_SIZE;
}

void freeAllocatedMemory(uint64_t index)
{
	uint8_t end = 0;
	uint64_t counter = 0;

	do
	{
		KHEAP_TABLE_ENTRY entry = getTableEntry(index + counter);
		if((*entry & KHEAP_FLAG_NEXT) == 0)
		{
			end = 1;
		}
		*entry &= KHEAP_FLAG_FREE;

		counter++;
	} while(!end);
}

int kzfreePage(void* pointer)
{
	int ret = 0;
	
	if(checkAlignment((uint64_t)pointer) < 0)
	{
		ret = -EIARG;
		goto out;
	}
	
	uint64_t index = addressToTableIndex(pointer);
	if(index < 0)
	{
		ret = -EIARG;
		goto out;
	}

	KHEAP_TABLE_ENTRY entry = getTableEntry(index);
	if((*entry & KHEAP_FLAG_START) == 0)
	{
		ret = -EIARG;
		goto out;
	}
	
	freeAllocatedMemory(index);
out:
	return ret;
}

int kheapInitPage()
{
	return kheapInitTable();	
}

#include "kheap_buddy.h"

#include <stddef.h>

#include "config.h"
#include "kernel.h"
#include "memory/memory.h"
#include "print.h"
#include "memory/paging/paging.h"
#include "status.h"
#include "stdint.h"

struct blockEntry* lvlTable[KHEAP_MAX_ALLOWED_LEVEL];
struct tableMetaData tableMetaData[KHEAP_MAX_ALLOWED_LEVEL];

uint8_t* tableStartAddress = NULL;
uint64_t lowerMemorySize = 0;
uint64_t heapSize = 0;

uint16_t maxLvl = 0;
uint64_t maxPage = 0;
uint64_t minPage = 0;

static uint64_t toNextLvl(uint64_t n)
{
	if(n == 0){ return 1; }

	uint64_t ret = 1;
	n--;
	
	while(n > 0)
	{
		n >>=1;
		ret<<=1;
	}
	return ret;
}

static uint16_t detectMaxLvl(uint64_t maxPage)
{
	uint64_t ret = 0;
	uint64_t curPage = maxPage;
	for(uint16_t i = 1; i < KHEAP_MAX_ALLOWED_LEVEL; i++)
	{
		if(curPage == BOBAOS_KERNEL_HEAP_SMALLEST_PAGE)
		{
			ret = i;
			break;
		}
		curPage /=2;
	}
	return ret;
}

static void initLvlTable()
{
	for(uint8_t i = 0; i < maxLvl; i++)
	{
		lvlTable[i] = (struct blockEntry*)(tableMetaData[i].tableStartAddress);
	}

	for(uint64_t i = 0; i < tableMetaData[0].maxEntries; i++)
	{
		struct blockEntry* entry = lvlTable[0]+i;
		entry->lvl = 0;
		entry->flags |= KHEAP_B_FLAG_FREE;
		
		if(i > 0)
		{
			entry->prev = (lvlTable[0] + (i-1));
		}

		if(i+1 != tableMetaData[0].maxEntries)
		{
			entry->next = (lvlTable[0] + (i+1));
		}
	}
}

static int readMetaData()
{
	uint64_t currentBlockSize = maxPage;
	uint64_t currentAddress = (uint64_t)tableStartAddress;
	for(uint16_t i = 0; i < maxLvl; i++)
	{
		struct tableMetaData* data = &tableMetaData[i];

		data->blockSize = currentBlockSize;
		data->maxEntries = heapSize/currentBlockSize;
		data->tableSize = data->maxEntries * sizeof(struct blockEntry);
		data->tableStartAddress = currentAddress;
		
		currentAddress += data->tableSize;
		currentBlockSize >>= 1;
	
		kprintf("  [%u] Block: %x Entries: %u TableSize: %x TableStart: %x\n", i, data->blockSize, data->maxEntries, data->tableSize, data->tableStartAddress);
	}
	
	if(tableMetaData[maxLvl-1].tableStartAddress + tableMetaData[maxLvl-1].tableSize > lowerMemorySize)
	{
		return -ENMEM;
	}
	return 0;
}

int kheapBInit()
{
	tableStartAddress = (uint8_t*)BOBAOS_KERNEL_HEAP_TABLE_ADDRESS;
	lowerMemorySize = getMaxMemorySize() - getUpperMemorySize();

	//Skip one enty, so that if the start address is at 0x0 we dont get problems later, becaues 0x0 is the NULL pointer which is used for error stuff
	if(tableStartAddress == NULL)
	{
		tableStartAddress += sizeof(struct blockEntry);
	}
	
	heapSize = toNextLvl((uint64_t)BOBAOS_KERNEL_HEAP_SIZE);
	maxPage = heapSize;
	while(maxPage != BOBAOS_KERNEL_HEAP_BIGGEST_PAGE)
	{
		maxPage/=2;
	}
	
	maxLvl = detectMaxLvl(maxPage);
	
	minPage = maxPage;
	for(uint64_t i = 1; i < maxLvl; i++)
	{
		minPage /=2;
	}
	
	if(minPage != BOBAOS_KERNEL_HEAP_SMALLEST_PAGE)
	{
		return -ENMEM;
	}
	
	kprintf("Kernel heap\n");	
	kprintf("  Biggest page: %x Smallest page: %x Level: %u\n\n", maxPage, minPage, maxLvl);

	if(readMetaData() < 0)
	{
		return -ENMEM;
	}
	
	memset((void*)tableStartAddress, 0x0, lowerMemorySize); 
	initLvlTable();	

	return 0;
}

static uint64_t lvlToBlockSize(uint16_t lvl)
{
	return tableMetaData[lvl].blockSize;
}

static int findLvlForBlock(uint64_t block)
{
	int searchLvl = maxLvl-1;

	while(searchLvl >= 0)
	{
		if(tableMetaData[searchLvl].blockSize == block){break;}

		searchLvl--;
	}

	if(searchLvl < 0)
	{
		return -ENFOUND;
	}
	return searchLvl;
}

static void* getHigherTableEntryAddressFromBlock(struct blockEntry* entry)
{
	if(entry->lvl+1 == maxLvl)
	{
		//Cant go up higher
		return NULL;
	}

	uint64_t currentTableIndex = ((uint64_t)entry - (uint64_t)lvlTable[entry->lvl]) / sizeof(struct blockEntry);
	uint64_t nextTableIndex = currentTableIndex*2;

	return (void*)(lvlTable[entry->lvl+1] + (nextTableIndex * sizeof(struct blockEntry)));
}

static struct blockEntry* getFreeEntry(uint16_t lvl)
{
	if((lvlTable[lvl]->flags & KHEAP_B_FLAG_HAS_ENTRIES) != 0)
	{
		return lvlTable[lvl]->next;
	}

	if((lvlTable[lvl]->flags & KHEAP_B_FLAG_FREE) != 0)
	{
		return lvlTable[lvl];
	}
	return NULL;
}

static void* entryToHeapAddress(struct blockEntry* entry)
{
	uint64_t index = ((uint64_t)entry - (uint64_t)lvlTable[entry->lvl]) / sizeof(struct blockEntry);
	uint64_t blockSize = lvlToBlockSize(entry->lvl);

	return (void*)((uint64_t)BOBAOS_KERNEL_HEAP_ADDRESS + (index * blockSize));
}

static int removeEntry(struct blockEntry* entry)
{
	if((entry->flags & KHEAP_B_FLAG_FREE) != 0)
	{
		entry->flags -= KHEAP_B_FLAG_FREE;	
		if(entry != lvlTable[entry->lvl])
		{
			if(entry->prev != NULL)
			{
				entry->prev->next = entry->next;
			}
		
			if(entry->next != NULL)
			{
				entry->next->prev = entry->prev;
			}

			if(entry->prev == NULL && entry->next == NULL)
			{
				//LIST IS NOW TOTALLY EMPTY OR FULL
				lvlTable[entry->lvl]->flags -= KHEAP_B_FLAG_HAS_ENTRIES;
			}
		}
		else
		{
			if(entry->next != NULL)
			{
				entry->next->prev = NULL;
				entry->flags |= KHEAP_B_FLAG_HAS_ENTRIES;
			}
		}
		return 0;
	}

	return -EIARG;
}

static struct blockEntry* splitEntryUp(uint16_t lvl)
{
	if(lvl+1 == maxLvl)
	{
		//Cant split up higher
		return NULL;
	}

	struct blockEntry* entry = lvlTable[lvl];
	if((entry->flags & KHEAP_B_FLAG_FREE) == 0)
	{
		if((entry->flags & KHEAP_B_FLAG_HAS_ENTRIES) != 0)
		{
			entry = entry->next;
		}
		else
		{
			return NULL;
		}
	}

	struct blockEntry* newStartAddr = (struct blockEntry*)getHigherTableEntryAddressFromBlock(entry);	
	
	//We maybe dont have enough memory here in the lower memory region.
	if((newStartAddr+1) >= lvlTable[entry->lvl+2])
	{
		return NULL;
	}

	removeEntry(entry);
		
	//Write 2 new entries in next Table
	for(uint8_t i = 0; i < 2; i++)
	{
		struct blockEntry* newEntry = (newStartAddr + i);
		
		if(newEntry == lvlTable[entry->lvl+1])
		{
			//First entry in new table
			
			newStartAddr->flags |= KHEAP_B_FLAG_HAS_ENTRIES;
			newEntry->next = newEntry+1;
			newEntry->prev = NULL;
		}
		else
		{
			if(i == 0)
			{
				if((lvlTable[entry->lvl+1]->flags & KHEAP_B_FLAG_FREE) != 0)
				{
					newEntry->prev = lvlTable[entry->lvl+1];
				}
				else
				{
					newEntry->prev = NULL;
				}
				newEntry->next = newEntry+1;
			}
			else
			{
				newEntry->prev = newEntry-1;
				struct blockEntry* temp = getFreeEntry(entry->lvl+1);
				if(temp != newEntry && temp != newEntry-1 && temp != lvlTable[entry->lvl+1])
				{
					newEntry->next = temp;
				}
				lvlTable[entry->lvl+1]->next = newEntry;
			}
			
			//We now wrote new enttries so now we have entries in this list, if we didnt had before
			lvlTable[entry->lvl+1]->flags |= KHEAP_B_FLAG_HAS_ENTRIES;
		}
		newEntry->lvl = entry->lvl+1;
		newEntry->flags |= KHEAP_B_FLAG_FREE;
	}

	return newStartAddr;
}

static struct blockEntry* splitAndReturn(uint16_t startLvl, uint16_t endLvl)
{
	struct blockEntry* entry = NULL;
	for(uint16_t i = startLvl; i < endLvl; i++)
	{
		entry = splitEntryUp(i);
	}

	return entry;	
}

void* kzBalloc(uint64_t size)
{
	void* ret = (void*)0x0;

	if(size == 0 || size > maxPage)
	{
		return ret;
	}
	
	uint64_t requestedBlock = toNextLvl(size);
	int searchLvl = findLvlForBlock(requestedBlock);

	//Search for block
	int freeLvl = searchLvl;
	while(freeLvl >= 0)
	{	
		if(((lvlTable[freeLvl]->flags & KHEAP_B_FLAG_FREE) != 0) || ((lvlTable[freeLvl]->flags & KHEAP_B_FLAG_HAS_ENTRIES) != 0)) {break;}
		freeLvl--;
	}
	
	//No more free blocks
	if(freeLvl < 0)
	{
		return ret;
	}

	struct blockEntry* takeEntry = NULL;
	if(searchLvl != freeLvl)
	{
		// Split and take it
		takeEntry = splitAndReturn(freeLvl, searchLvl);
	} 
	else
	{
		//Just take it
		takeEntry = getFreeEntry(searchLvl);
	}

	removeEntry(takeEntry);
	return entryToHeapAddress(takeEntry);
}

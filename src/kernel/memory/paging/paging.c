#include "paging.h"

#include <macros.h>
#include <stddef.h>

#include "config.h"
#include "memory/memory.h"
#include "status.h"
#include "memory/kheap/kheap.h"
#include "print.h"

struct memoryMap memoryMap[BOBAOS_MEMORY_MAP_MAX_ENTRIES];
uint8_t memoryMapLength = 0x0;

uint64_t upperMemorySize = 0x0;
uint64_t totalMemorySize = 0x0;

uint64_t total4KBPagesMapped = 0;

static void writePointerTableEntry(uint64_t* dest, const uint64_t* src, uint64_t index, uint16_t flags)
{
	dest[index] = ((uint64_t)src & 0xFFFFFFFFF000) | flags;	
}

static uint64_t downToPage(uint64_t var)
{
	if ((var % SIZE_4KB) == 0)
		return var;
	return var - (var % SIZE_4KB);
}

static uint64_t upToPage(uint64_t var)
{
	if ((var % SIZE_4KB) == 0)
		return var;
	return var + SIZE_4KB - (var % SIZE_4KB);
}

static uint64_t* returnTableEntryNoFlags(const uint64_t* table, uint16_t index)
{
	return (uint64_t*)(table[index] & 0xFFFFFFFFFFFFF000);
}

static uint16_t getPml4IndexFromVirtual(void* virt)
{
	return ((uint64_t)virt >> SHIFT_PML4) & FULL_512;
}

static uint16_t getPdpIndexFromVirtual(void* virt)
{
	return ((uint64_t)virt >> SHIFT_PDP) & FULL_512;
} 

static uint16_t getPdIndexFromVirtual(void* virt)
{
	return ((uint64_t)virt >> SHIFT_PD) & FULL_512;
}

static uint16_t getPtIndexFromVirtual(void* virt)
{
	return ((uint64_t)virt >> SHIFT_PT) & FULL_512;
}

static PTTable getPTTable(void* virt, PML4Table table)
{
	uint16_t pdpIndex = ((uint64_t)virt >> SHIFT_PDP) & FULL_512;
	uint16_t pdIndex = ((uint64_t)virt >> SHIFT_PD) & FULL_512;
	
	PDPTable pdpTable = (PDPTable)returnTableEntryNoFlags(table, getPml4IndexFromVirtual(virt));
	RETNULL(pdpTable);

	PDTable pdTable = (PDTable)returnTableEntryNoFlags(pdpTable, pdpIndex);
	RETNULL(pdTable);

	PTTable ptTable = (PTTable)returnTableEntryNoFlags(pdTable, pdIndex);
	return ptTable;
}

//If an entry does not exist, this function will create one with the flags P and RW
static uint64_t getOrCreateEntry(uint64_t* table, uint16_t entryIndex)
{
	if (table[entryIndex] == 0)
	{
		uint64_t* newEntry = (uint64_t*)kzalloc(4096);
		if (newEntry == NULL)
			return 0;

		writePointerTableEntry(table, newEntry, entryIndex, PAGING_FLAG_P | PAGING_FLAG_RW);
		return (uint64_t)newEntry;
	}
	return table[entryIndex];
}

int mapOrCreateRange(PML4Table pml4Table, uint64_t physStart, uint64_t virtStart, uint64_t size)
{
	int pageCounter = 0;

	uint64_t sizeMapped = 0;
	do {
		void* virt = (void*)(virtStart + pageCounter * SIZE_4KB);
		void* phys = (void*)(physStart + pageCounter * SIZE_4KB);

		uint16_t pml4Index = getPml4IndexFromVirtual(virt);
		uint16_t pdpIndex = getPdpIndexFromVirtual(virt);
		uint16_t pdIndex = getPdIndexFromVirtual(virt);
		uint16_t ptIndex = getPtIndexFromVirtual(virt);

		PDPTable pdpTable = (PDPTable)(getOrCreateEntry(pml4Table, pml4Index) & 0xFFFFFFFFFFFFF000);
		RETNULLERROR(pdpTable, -ENMEM);

		PDTable pdTable = (PDTable)(getOrCreateEntry(pdpTable, pdpIndex) & 0xFFFFFFFFFFFFF000);
		RETNULLERROR(pdTable, -ENMEM);

		PTTable ptTable = (PTTable)(getOrCreateEntry(pdTable, pdIndex) & 0xFFFFFFFFFFFFF000);
		RETNULLERROR(ptTable, -ENMEM);

		writePointerTableEntry(ptTable, phys, ptIndex, PAGING_FLAG_P | PAGING_FLAG_RW);

		pageCounter++;
		sizeMapped += SIZE_4KB;
	} while (sizeMapped < size);

	return SUCCESS;
}

//The problem with the memory map is that the lower 1MB are not correctly page aligned. Not the size nor the offset.
// So we just merge together the lower memory map entries and the 'createKernelTable' function will correclty map 1Mib
static void reworkMemoryMap()
{
	uint8_t counter = 0;

	struct memoryMap* currMap = &memoryMap[0];
	while (currMap->type != 0 && currMap->address < 0x100000)
	{
		counter++;
		currMap = &memoryMap[counter];
	}

	memset(memoryMap, 0x0, counter*sizeof(struct memoryMap));

	//1 entry for lower memory region
	memoryMap[0].address = 0x0;
	memoryMap[0].length = 0x100000;
	memoryMap[0].type = MEMORY_TYPE_FREE;

	counter--;
	if (counter > 0)
	{
		memcpy(&memoryMap[1], memoryMap+counter+1, sizeof(struct memoryMap) * (memoryMapLength-counter));
		memoryMapLength -= counter;
	}
}

PML4Table createKernelTable(uint64_t virtualStart)
{
	print("Paging: \n");

	PML4Table pml4Table = (PML4Table)kzalloc(4096);
	RETNULL(pml4Table);

	uint64_t sizeMapped = 0;
	//uint64_t usableSize = downToPage(size);

	for (uint8_t i = 0; i < memoryMapLength; i++)
	{
		if (memoryMap[i].type == MEMORY_TYPE_FREE)
		{
			uint64_t physicalStart = downToPage(memoryMap[i].address);
			uint64_t rangeSize = downToPage(memoryMap[i].length);

			int ret = mapOrCreateRange(pml4Table, physicalStart, virtualStart + sizeMapped, rangeSize);
			if (ret < 0)
			{
				//Error, kernel heap is full
				goto error;
			}

			kprintf("  Mapped %x -> %x, Length: %x\n", physicalStart, virtualStart+sizeMapped, rangeSize);
			sizeMapped += rangeSize;
		}
	}
	print("\n");

	loadNewPageTable(pml4Table);
	return pml4Table;

	error:
	kzfree(pml4Table);
	return NULL;
}

//Takes a virtual address and the PLM4 paging table and returns the physical address for the given virtual one
void* virtualToPhysical(void* virt, PML4Table table)
{	
	PTTable ptTable = getPTTable(virt, table);
	RETNULL(ptTable);

	uint16_t ptIndex = ((uint64_t)virt >> SHIFT_PT) & FULL_512;

	uint16_t offset = (uint64_t)virt & FULL_4KB;	
	uint64_t address = (uint64_t)returnTableEntryNoFlags(ptTable, ptIndex);

	return (void*)(address + offset);
}

int remapPhysicalToVirtual(void* physical, void* virtual, PML4Table table)
{
	PTTable oldPt = getPTTable(virtual, table);
	RETNULLERROR(oldPt, -ENFOUND);

	uint16_t oldPtIndex = ((uint64_t)virtual >> SHIFT_PT) & FULL_512;
	writePointerTableEntry(oldPt, (void*)downToPage((uint64_t)physical), oldPtIndex, PAGING_FLAG_P | PAGING_FLAG_RW);

	return SUCCESS;
}

int remapPhysicalToVirtualRange(void* physical, void* virtual, uint64_t size, PML4Table table)
{
	uint64_t blocks = upToPage(size) / SIZE_4KB;
	for (uint64_t i = 0; i < blocks; i++)
	{
		if (remapPhysicalToVirtual(physical + (i*SIZE_4KB), virtual + (i*SIZE_4KB), table) > 0)
			return -ENMEM;
	}
	return SUCCESS;
}

int remapVirtualToVirtual(void* to, void* from, PML4Table table)
{
	void* fromPhysical = virtualToPhysical(from, table);
	return mapOrCreateRange(table, downToPage((uint64_t)fromPhysical), downToPage((uint64_t)to), SIZE_4KB);
}

int remapVirtualToVirtualRange(void* to, void* from, uint64_t size, PML4Table table)
{
	uint64_t blocks = upToPage(size) / SIZE_4KB;
	for (uint64_t i = 0; i < blocks; i++)
	{
		if (remapVirtualToVirtual(to + (i*SIZE_4KB), from + (i*SIZE_4KB), table) > 0)
			return -ENMEM;
	}
	return SUCCESS;
}

void removePageFlag(void* virtual, uint16_t flag, PML4Table table)
{
	PTTable ptTable = getPTTable(virtual, table);
	uint16_t ptIndex = getPtIndexFromVirtual(virtual);
	ptTable[ptIndex] &= ~flag;
}

uint64_t getUpperMemorySize()
{
	return upperMemorySize;
}
uint64_t getMaxMemorySize()
{
	return totalMemorySize;
}

void readMemoryMap()
{
	uint8_t* mapAddress = (uint8_t*) 0xF000;
	uint16_t offset = 0x0;
	
	uint8_t length = 0;
	do
	{
		struct memoryMap* map = (struct memoryMap*)(mapAddress + offset);
		if(!map->type){break;}	
		
		memcpy(((uint8_t*)memoryMap)+offset, (void*)(mapAddress+offset), sizeof(struct memoryMap));

		offset += sizeof(struct memoryMap);
		length++;

		totalMemorySize += map->length;
		if(map->address >= SIZE_1MB)
		{
			upperMemorySize += map->length;
		}
	} while(1);
	memoryMapLength = length;
	reworkMemoryMap();

	//for (uint8_t i = 0; i < memoryMapLength; i++)
		//kprintf("Address: %x, Length: %x, Type: %x\n", memoryMap[i].address, memoryMap[i].length, memoryMap[i].type);
}

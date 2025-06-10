#include "paging.h"

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
	return var - (var % SIZE_4KB);
}

static uint64_t* returnTableEntryNoFlags(const uint64_t* table, uint16_t index)
{
	return (uint64_t*)(table[index] & 0xFFFFFFFFFFFFF000);
}

static int sizeToPdpEntriesUp(uint64_t size)
{
	if(size > 0x8000000000)
	{
		return -EIARG;	
	}

	if(size < SIZE_1GB)
	{
		return -ENMEM;
	}	

	uint64_t downSize = downToPage(size);
	int pdEntries = (int)(downSize / (uint64_t)SIZE_1GB);

	if (downSize % SIZE_1GB > 0 )
		pdEntries++;
	return pdEntries;
}

static uint16_t getPml4IndexFromVirtual(void* virt)
{
	return ((uint64_t)virt >> SHIFT_PML4) & FULL_512;
}

static uint16_t getPdpIndexFromVirtual(void* virt)
{
	return ((uint64_t)virt >> SHIFT_PDP) & FULL_512;
} 
/*
static uint16_t getPdIndexFromVirtual(void* virt)
{
	return ((uint64_t)virt >> SHIFT_PD) & FULL_512;
}
*/
static uint16_t getPtIndexFromVirtual(void* virt)
{
	return ((uint64_t)virt >> SHIFT_PT) & FULL_512;
}

static PTTable getPTTable(void* virt, PML4Table table)
{
	uint16_t pdpIndex = ((uint64_t)virt >> SHIFT_PDP) & FULL_512;
	uint16_t pdIndex = ((uint64_t)virt >> SHIFT_PD) & FULL_512;
	
	PDPTable pdpTable = (PDPTable)returnTableEntryNoFlags(table, getPml4IndexFromVirtual(virt));
	PDTable pdTable = (PDTable)returnTableEntryNoFlags(pdpTable, pdpIndex);
	PTTable ptTable = (PTTable)returnTableEntryNoFlags(pdTable, pdIndex);
	
	return ptTable;
}

static PTTable createPTTable(uint64_t phys)
{
	PTTable table = (PTTable)kzalloc(4096);
	if(table == NULL){return NULL;}

	for(uint16_t i = 0; i < 512; i++)
	{
		total4KBPagesMapped++;
		writePointerTableEntry(table, (uint64_t*)(phys + (i*SIZE_4KB)), i, PAGING_FLAG_P | PAGING_FLAG_RW);
	}

	return table;
} 

static PDTable createPdTable(uint64_t phys, uint16_t entries)
{
	PDTable table = (PDTable)kzalloc(4096);
	if(table == NULL){return NULL;}

	for(uint16_t i = 0; i < entries; i++)
	{
		PTTable ptTable = createPTTable(phys + (i*(SIZE_1MB*2)));
		if(ptTable == NULL){return NULL;}
		writePointerTableEntry(table, ptTable, i, PAGING_FLAG_P | PAGING_FLAG_RW);
	}	

	return table;
} 

PML4Table createKernelTable(uint64_t physical, uint64_t virtual, uint64_t size)
{
	print("Paging: \n");
	uint64_t usableSize = downToPage(size);

	PML4Table pml4Table = (PML4Table)kzalloc(4096);
	PDPTable pdpTable = (PDPTable)kzalloc(4096);
	
	if(pml4Table == NULL || pdpTable == NULL)
	{
		kzfree(pml4Table);
		kzfree(pdpTable);

		return NULL;
	}

	writePointerTableEntry(pml4Table, pdpTable, getPml4IndexFromVirtual((void*)virtual), PAGING_FLAG_P | PAGING_FLAG_RW);
	
	int pdpEntries = sizeToPdpEntriesUp(usableSize);
	if(pdpEntries <= 0)
	{
		return NULL;
	}

	for(int i = 0; i < pdpEntries; i++)
	{
		uint64_t virt = virtual + (i*(uint64_t)SIZE_1GB);
		uint64_t phys = physical + (i*(uint64_t)SIZE_1GB);

		uint16_t pdEntries = 512;
		if ((i+1) == pdpEntries)
		{
			uint64_t restSize = size - (i*(uint64_t)SIZE_1GB);
			pdEntries = restSize / (((uint64_t)SIZE_4KB)*512);
		}

		PDTable pdTable = createPdTable(phys, pdEntries);
		if(pdTable == NULL){return NULL;}
		uint16_t pdpIndex = getPdpIndexFromVirtual((void*)virt);
		writePointerTableEntry(pdpTable, pdTable, pdpIndex, PAGING_FLAG_P | PAGING_FLAG_RW);

		kprintf("  Mapped %x:%x -> %x:%x\n", phys, phys+(pdEntries*(SIZE_4KB*512)), virt, virt+(pdEntries*(SIZE_4KB*512)));
	}
	print("\n");

	kprintf("  Total 4KB pages mapped: %u, Total size mapped %x\n", total4KBPagesMapped, total4KBPagesMapped * (uint64_t)SIZE_4KB);
	kprintf("  Wasting: %x of memory\n\n", size - (total4KBPagesMapped * (uint64_t)SIZE_4KB));
	loadNewPageTable(pml4Table);
	return pml4Table;
}

//Takes a virtual address and the PLM4 paging table and returns the physical address for the given virtual one
void* virtualToPhysical(void* virt, PML4Table table)
{	
	PTTable ptTable = getPTTable(virt, table);
	uint16_t ptIndex = ((uint64_t)virt >> SHIFT_PT) & FULL_512;

	uint16_t offset = (uint64_t)virt & FULL_4KB;	
	uint64_t address = (uint64_t)returnTableEntryNoFlags(ptTable, ptIndex);

	return (void*)(address + offset);
}

void remapPage(void* to, void* from, PML4Table table)
{
	PTTable oldPt = getPTTable(from, table);	
	uint16_t oldPtIndex = ((uint64_t)from >> SHIFT_PT) & FULL_512;

	writePointerTableEntry(oldPt, to, oldPtIndex, PAGING_FLAG_P | PAGING_FLAG_RW);
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

		if(map->type == MEMORY_TYPE_FREE)
		{
			totalMemorySize += map->length;
			if(map->address >= SIZE_1MB)
			{
				upperMemorySize += map->length;
			}
		}
	} while(1);
	memoryMapLength = length;
}

#include "memory/paging/paging.h"

#include "status.h"
#include "memory/kheap/kheap.h"

static void writePointerTableEntry(uint64_t* dest, uint64_t* src, uint64_t index, uint16_t flags)
{
	dest[index] = ((uint64_t)src & 0xFFFFFFFFF000) | flags;	
}

static uint64_t downToPage(uint64_t var)
{
	return var - (var % 4096);
}

static uint64_t* returnTableEntryNoFlags(uint64_t* table, uint16_t index)
{
	return (uint64_t*)(table[index] & 0xFFFFFFFFFFFFF000);
}

static int sizeToPdEntries(uint64_t size)
{
	if(size > 0x8000000000)
	{
		return -ENMEM;	
	}

	if(size < 0x40000000)
	{
		return -EIARG;
	}	

	return downToPage(size) / 0x40000000;
}

static uint16_t getPlm4IndexFromVirtual(void* virt)
{
	return ((uint64_t)virt >> 0x27) & 0x1FF;
}

static uint16_t getPdpIndexFromVirtual(void* virt)
{
	return ((uint64_t)virt >> 0x1E) & 0x1FF;
} 

static PTTable getPTTable(void* virt, PLM4Table table)
{
	uint16_t pdpIndex = ((uint64_t)virt >> 0x1E) & 0x1FF;
	uint16_t pdIndex = ((uint64_t)virt >> 0x15) & 0x1FF;
	
	PDPTable pdpTable = (PDPTable)returnTableEntryNoFlags(table, getPlm4IndexFromVirtual((void*)virt));
	PDTable pdTable = (PDTable)returnTableEntryNoFlags(pdpTable, pdpIndex);
	PTTable ptTable = (PTTable)returnTableEntryNoFlags(pdTable, pdIndex);
	
	return ptTable;
}

static PTTable createPTTable(uint64_t phys)
{
	PTTable table = (PTTable)kzalloc(4096);
	
	for(uint16_t i = 0; i < 512; i++)
	{
		writePointerTableEntry(table, (uint64_t*)(phys + (i*0x1000)), i, PAGING_FLAG_P | PAGING_FLAG_RW | PAGING_FLAG_PS);
	}

	return table;
} 

static PDTable createPdTable(uint64_t phys, uint64_t virt)
{
	PDTable table = (PDTable)kzalloc(4096);
	
	for(uint16_t i = 0; i < 512; i++)
	{
		PTTable ptTable = createPTTable(phys + (i*0x200000));
		writePointerTableEntry(table, ptTable, i, PAGING_FLAG_P | PAGING_FLAG_RW);
	}	

	return table;
} 

PLM4Table createKernelTable(uint64_t physical, uint64_t virtual, uint64_t size)
{
	uint64_t usableSize = downToPage(size);

	PLM4Table plm4Table = (PLM4Table)kzalloc(4096);
	PDPTable pdpTable = (PDPTable)kzalloc(4096);
	
	writePointerTableEntry(plm4Table, pdpTable, getPlm4IndexFromVirtual((void*)virtual), PAGING_FLAG_P | PAGING_FLAG_RW);
	
	uint64_t pdEntries = sizeToPdEntries(usableSize);
	//if pd < 0 then panic for now lol
	
	for(uint16_t i = 0; i < pdEntries; i++)
	{
		uint64_t virt = virtual + (i*(uint64_t)0x40000000);

		PDTable pdTable = createPdTable(physical + i*(uint64_t)0x40000000, virt);
		uint16_t pdpIndex = getPdpIndexFromVirtual((void*)virt);
		writePointerTableEntry(pdpTable, pdTable, pdpIndex, PAGING_FLAG_P | PAGING_FLAG_RW);
	}
	
	loadNewPageTable(plm4Table);
	return plm4Table;
}

//Takes a virtual address and the PLM4 paging table and returns the physical address for the given virtual one
void* virtualToPhysical(void* virt, PLM4Table table)
{	
	PTTable ptTable = getPTTable(virt, table);
	uint16_t ptIndex = ((uint64_t)virt >> 0xC) & 0x1FF;

	uint16_t offset = (uint64_t)virt & 0x0FFF;	
	uint64_t address = (uint64_t)returnTableEntryNoFlags(ptTable, ptIndex);

	return (void*)(address + offset);
}


void remapPage(void* to, void* from, PLM4Table table)
{
	PTTable oldPt = getPTTable(from, table);	
	uint16_t oldPtIndex = ((uint64_t)from >> 0xC) & 0x1FF;

	writePointerTableEntry(oldPt, to, oldPtIndex, PAGING_FLAG_P | PAGING_FLAG_RW | PAGING_FLAG_PS);	
}

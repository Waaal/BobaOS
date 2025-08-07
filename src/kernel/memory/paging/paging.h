#ifndef PAGING_H
#define PAGING_H

#include <stdint.h>

#define PAGING_FLAG_P 	0b0000000000000001
#define PAGING_FLAG_RW	0b0000000000000010	
#define PAGING_FLAG_US	0b0000000000000100	
#define PAGING_FLAG_PS	0b0000000010000000	

#define SHIFT_PML4 0x27
#define SHIFT_PDP 0x1E
#define SHIFT_PD 0x15
#define SHIFT_PT 0xC

#define SIZE_1KB 0x400
#define SIZE_4KB 0x1000
#define SIZE_1MB 0x100000
#define SIZE_1GB 0x40000000

#define FULL_512 0x1FF
#define FULL_4KB 0xFFF
#define FULL_1GB 0x3FFFFFFF

#define MEMORY_TYPE_FREE 0x1
#define MEMORY_TYPE_RESERVED 0x2

struct memoryMap
{
	uint64_t address;
	uint64_t length;
	uint32_t type;
} __attribute__((packed));

// PML4 -> PDP -> PD -> PT

// 1 PT = 0x200000 (2048 KB) of memory
// 1 PD = 0x40000000 (1024 MB) of memory
// 1 PDP = 0x8000000000 (512 GB) of memory

typedef uint64_t* PML4Table;
typedef uint64_t* PDPTable;
typedef uint64_t* PDTable;
typedef uint64_t* PTTable;

void readMemoryMap();
uint64_t getUpperMemorySize();
uint64_t getMaxMemorySize();

uint64_t downToPage(uint64_t var);
uint64_t upToPage(uint64_t var);

PML4Table createKernelTable(uint64_t virtualStart);
void* virtualToPhysical(void* virt, PML4Table table);
void removePageFlag(void* virtual, uint16_t flag, PML4Table table);
int remapPhysicalToVirtual(void* physical, void* virtual, PML4Table table);
int remapPhysicalToVirtualRange(void* physical, void* virtual, uint64_t size, PML4Table table);
int remapVirtualToVirtual(void* to, void* from, PML4Table table);
int remapVirtualToVirtualRange(void* to, void* from, uint64_t size, PML4Table table);
struct memoryMap* getMemoryMapForAddress(uint64_t address);


// Functions in paging.asm
void loadNewPageTable(PML4Table table);

#endif

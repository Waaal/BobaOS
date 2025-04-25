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

#define SIZE_4KB 0x1000
#define SIZE_2MB 0x200000
#define SIZE_1GB 0x40000000

#define FULL_512 0x1FF
#define FULL_4KB 0xFFF

// PML4 -> PDP -> PD -> PT

// 1 PT = 0x200000 (2048 KB) of memory
// 1 PD = 0x40000000 (1024 MB) of memory
// 1 PDP = 0x8000000000 (512 GB) of memory

typedef uint64_t* PML4Table;
typedef uint64_t* PDPTable;
typedef uint64_t* PDTable;
typedef uint64_t* PTTable;

PML4Table createKernelTable(uint64_t physical, uint64_t virtual, uint64_t size);
void* virtualToPhysical(void* virt, PML4Table table);
void remapPage(void* to, void* from, PML4Table table);

void loadNewPageTable(PML4Table table); // function in paging.asm

#endif

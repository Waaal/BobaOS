#ifndef PAGING_H
#define PAGING_H

#include "stdint-gcc.h"

#define PAGING_FLAG_P 	0b0000000000000001
#define PAGING_FLAG_RW	0b0000000000000010	
#define PAGING_FLAG_US	0b0000000000000100	
#define PAGING_FLAG_PS	0b0000000010000000	

// PML4 -> PDP -> PD -> PT

// 1 PT = 0x200000 (2048 KB) of memory
// 1 PD = 0x40000000 (1024 MB) of memory
// 1 PDP = 0x8000000000 (512 GB) of memory

typedef uint64_t* PLM4Table;
typedef uint64_t* PDPTable;
typedef uint64_t* PDTable;
typedef uint64_t* PTTable;

PLM4Table createKernelTable(uint64_t physical, uint64_t virtual, uint64_t size);
void* virtualToPhysical(void* virt, PLM4Table table);
void remapPage(void* to, void* from, PLM4Table table);

void loadNewPageTable(PLM4Table table); // function in paging.asm

#endif

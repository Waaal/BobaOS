#ifndef H_KHEAP_PAGE
#define H_KHEAP_PAGE

#include <stdint.h>


#define KHEAP_FLAG_FREE		0x0
#define KHEAP_FLAG_LOCKED	0x1
#define KHEAP_FLAG_START	0x40
#define KHEAP_FLAG_NEXT		0x80

#define KHEAP_BLOCK_SIZE 4096 //4KB

typedef unsigned char* KHEAP_TABLE_ENTRY;

int kheapInitPage();
void* kzallocPage(uint64_t size);
int kzfreePage(void* pointer); 

#endif

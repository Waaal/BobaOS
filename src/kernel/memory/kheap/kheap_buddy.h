#ifndef KHEAP_BUDDY_H
#define KHEAP_BUDDY_H

#include <stdint.h>

#define KHEAP_MAX_ALLOWED_LEVEL 32

#define KHEAP_B_FLAG_FREE 0x1
#define KHEAP_B_FLAG_SPLITUP 0x2
#define KHEAP_B_FLAG_ALLOCATED 0x4
#define KHEAP_B_FLAG_HAS_ENTRIES 0x80 

enum removeType
{
	REMOVE_TYPE_SPLIT,
	REMOVE_TYPE_ALLOCATED,
	REMOVE_TYPE_MERGE
};

struct tableMetaData
{
	uint64_t maxEntries;
	uint64_t tableSize;
	uint64_t tableStartAddress;
	uint64_t blockSize;
};

struct blockEntry
{
	struct blockEntry* next;
	struct blockEntry* prev;
	uint8_t lvl;
	uint8_t flags;
} __attribute__((packed));

int kheapInitBuddy();
void* kzallocBuddy(uint64_t size);
int kzfreeBuddy(void* pointer);

#endif

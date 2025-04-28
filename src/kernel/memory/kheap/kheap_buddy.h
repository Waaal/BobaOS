#ifndef KHEAP_BUDDY_H
#define KHEAP_BUDDY_H

#include <stdint.h>

#define KHEAP_MAX_ALLOWED_LEVEL 32

#define KHEAP_B_FLAG_FREE 0x1
#define KHEAP_B_FLAG_HAS_ENTRIES 0x80 //This entrie is use for the traverse of the linked list

struct blockEntry
{
	struct blockEntry* next;
	struct blockEntry* prev;
	uint8_t lvl;
	uint8_t flags;
} __attribute__((packed));

int kheapBInit();
void* kzBalloc(uint64_t size);

#endif

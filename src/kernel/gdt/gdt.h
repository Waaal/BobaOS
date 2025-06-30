#ifndef GDT_H
#define GDT_H

#include <stdint.h>

#define GDT_ACCESS_SYSTEM 0x10

struct gdt
{
	uint64_t limit;
	uint64_t base;
	uint8_t access;
	uint8_t flags;
};


struct realGdt
{
	uint16_t limit1;
	uint32_t base1_access;
	uint8_t limit2_flags;
	uint8_t base2;
		
} __attribute__((packed));

struct realGdtSystemPart
{
	uint32_t base;
	uint32_t reserved;
} __attribute__((packed));

struct gdtPtr
{
	uint16_t size;
	uint64_t offset;
} __attribute__((packed));

void loadGdt(struct gdt* gdt);

//functions in gdt.asm
void loadGdtPtr(struct gdtPtr* gdtPtrAddress);

#endif

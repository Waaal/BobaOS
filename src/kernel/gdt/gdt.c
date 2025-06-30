#include "gdt.h"

#include "config.h"
#include "memory/memory.h"

static uint8_t totalSystemDescriptors = 0;

static struct realGdt realGdt[BOBAOS_GDT_ENTRIES];
static struct gdtPtr gdtPtr = {
	.size = 0x0,
	.offset = 0x0
};


void toRealGdt(struct gdt* gdt)
{
	uint16_t realGdtCounter = 0;
	for(uint8_t i = 0; i < BOBAOS_GDT_ENTRIES; i++)
	{
		struct realGdt real = {
			.limit1 = gdt[i].limit & 0xFFFF,
			.base1_access = (gdt[i].base & 0xFFFFFF) | ((uint32_t)gdt[i].access << 24),
			.limit2_flags = ((gdt[i].limit >> 16) & 0xF) | (gdt[i].flags << 4),
			.base2 = (gdt[i].base >> 24) & 0xFF
		};
		memcpy((void*)((uint64_t)realGdt + (sizeof(struct realGdt) * realGdtCounter)), &real, sizeof(struct realGdt));
		realGdtCounter++;

		if (i != 0 && ((gdt[i].access) & GDT_ACCESS_SYSTEM) == 0)
		{
			//this is a system segment, need 128bit. So we use a second entry
			totalSystemDescriptors++;

			struct realGdtSystemPart realSystem = {
				.base = (gdt[i].base >> 32) & 0xFFFF,
				.reserved = 0x0
			};
			memcpy((void*)((uint64_t)realGdt + (sizeof(struct realGdt) * realGdtCounter)), &realSystem, sizeof(struct realGdtSystemPart));
			realGdtCounter++;
		}
	}	
} 

void loadGdt(struct gdt* gdt)
{
	toRealGdt(gdt);

	gdtPtr.size = (sizeof(struct realGdt) * (BOBAOS_GDT_ENTRIES+totalSystemDescriptors)) - 1;
	gdtPtr.offset = (uint64_t)realGdt;
	loadGdtPtr(&gdtPtr);
}

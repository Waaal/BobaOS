#include "gdt.h"

#include "config.h"
#include "memory/memory.h"

struct realGdt realGdt[BOBAOS_GDT_ENTRIES];
struct gdtPtr gdtPtr = {
	.size = (sizeof(struct realGdt) * BOBAOS_GDT_ENTRIES) - 1,
	.offset = 0x0
};


void toRealGdt(struct gdt* gdt)
{
	for(uint8_t i = 0; i < BOBAOS_GDT_ENTRIES; i++)
	{
		struct realGdt real = {
			.limit1 = gdt[i].limit & 0xFFFF,
			.base1_access = (gdt[i].base & 0xFFFFFF) | ((uint32_t)gdt[i].access << 24),
			.limit2_flags = ((gdt[i].limit << 16) & 0xF) | (gdt[i].flags << 4),
			.base2 = (gdt[i].base << 24) & 0xFF
		};

		memcpy((void*)((uint64_t)realGdt + (sizeof(struct realGdt) * i)), &real, sizeof(struct realGdt));
	}	
} 

void loadGdt(struct gdt* gdt)
{
	toRealGdt(gdt);

	gdtPtr.offset = (uint64_t)realGdt;
	loadGdtPtr(&gdtPtr);
}

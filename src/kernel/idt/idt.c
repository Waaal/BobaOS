#include "idt.h"

#include "config.h"
#include "memory/memory.h"
#include "terminal.h"

extern uint64_t* idtAddressList[BOBAOS_TOTAL_INTERRUPTS]; //from idt.asm

struct idtEntry idt[BOBAOS_TOTAL_INTERRUPTS];
struct idtPtr idtPtr;


void idtSet(uint16_t vector, void* address, enum idtGateType gateType, uint8_t dpl, uint16_t selector)
{
	struct idtEntry entry;

	entry.offset1 = (uint64_t)address & 0x000000000000FFFF;
	entry.offset2 = ((uint64_t)address >> 16) & 0x000000000000FFFF;
	entry.offset3 = ((uint64_t)address >> 32) & 0x00000000FFFFFFFF;

	entry.selector = selector;
	entry.attributes = 0x80 | (dpl << 5) | gateType;
	entry.resered_ist = 0x0;

	memcpy((void*)((uint64_t)idt + (vector*sizeof(struct idtEntry))), &entry, sizeof(struct idtEntry));
}

//sets up a IDT and all of its entries, maps them to functions and enables interrupts
void idtInit()
{
	memset(idt, 0x0, BOBAOS_TOTAL_INTERRUPTS * sizeof(struct idtEntry));
	memset(idtAddressList, 0x0, BOBAOS_TOTAL_INTERRUPTS * 0x8);

	DEBUG_STRAP_REMOVE_LATER();	
	kprintf("IDT VECTOR 32 ADDRESS: %x", idtAddressList[32]);
	
	idtSet(32, idtAddressList[32], TYPE_INTERRUPT, 0x0, BOBAOS_KERNEL_SELECTOR_CODE);
	
	idtPtr.offset = (uint64_t)idt;
	idtPtr.size = (sizeof(struct idtEntry) * BOBAOS_TOTAL_INTERRUPTS) - 1;

	loadIdtPtr(&idtPtr);
	enableInterrupts();
}

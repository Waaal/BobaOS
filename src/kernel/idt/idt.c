#include "idt.h"

#include "config.h"
#include "irqHandler.h"
#include "exceptionHandler.h"
#include "memory/memory.h"
#include "status.h"
#include "print.h"
#include "io/io.h"

extern uint64_t* idtAddressList[BOBAOS_TOTAL_INTERRUPTS]; //from idt.asm

struct idtEntry idt[BOBAOS_TOTAL_INTERRUPTS];
struct idtPtr idtPtr;

TRAPHANDLER handlerList[BOBAOS_TOTAL_INTERRUPTS];

void printTrapFrame(struct trapFrame* frame)
{
	kprintf("Trap frame: %x/n/n", frame);
	kprintf("RAX: %x  RBX: %x  RCX: %x  RDX: %x/n", frame->rax, frame->rbx, frame->rcx, frame->rdx);
	kprintf("RSI: %x  RDI: %x  RBP: %x/n/n", frame->rsi, frame->rdi, frame->rbp);
	kprintf("R15: %x  R14: %x  R13: %x  R12: %x/n", frame->r15, frame->r14, frame->r13, frame->r12);
	kprintf("R11: %x  R10: %x  R09: %x  R08: %x/n/n", frame->r11, frame->r10, frame->r9, frame->r8);
	kprintf("RIP: %x  CS: %x  FLAGS: %x  RSP: %x  DS: %x/n/n", frame->rip, frame->cs, frame->flags, frame->rsp, frame-> ds);
}

static void fillIdt()
{
	for(uint16_t i = 0; i < BOBAOS_TOTAL_INTERRUPTS; i++)
	{
		idtSet(i, idtAddressList[i], IDT_GATE_TYPE_INTERRUPT, 0, BOBAOS_KERNEL_SELECTOR_CODE);
	}
}

void idtSet(uint16_t vector, void* address, enum idtGateType gateType, uint8_t dpl, uint16_t selector)
{
	struct idtEntry entry;

	entry.offset1 = (uint64_t)address & 0x000000000000FFFF;
	entry.offset2 = ((uint64_t)address >> 16) & 0x000000000000FFFF;
	entry.offset3 = ((uint64_t)address >> 32) & 0x00000000FFFFFFFF;

	entry.selector = selector;
	entry.attributes = 0x80 | (dpl << 5) | gateType;
	entry.reserved_ist = 0x0;

	memcpy((void*)((uint64_t)idt + (vector*sizeof(struct idtEntry))), &entry, sizeof(struct idtEntry));
}

int8_t registerHandler(uint16_t vector, TRAPHANDLER handler)
{
	if(vector >= BOBAOS_TOTAL_INTERRUPTS)
	{
		return -EIARG;
	}

	handlerList[vector] = handler;
	return 0;
}

void trapHandler(uint16_t vector, uint32_t errorCode, struct trapFrame* frame)
{
	if(vector >= BOBAOS_TOTAL_INTERRUPTS)
	{
		kprintf("[ERROR] Trap handler got called with the illegal vector %u/n", vector);
		return;
	}
	
	if((uint64_t)(handlerList[vector]) == 0)
	{
		kprintf("[ERROR] No trap handler available for vector %u/n", vector);
	}

	if(handlerList[vector](frame) < 0)
	{
		kprintf("[ERROR] Trap handler %u returned an error/n");
	}

	if(vector > 31 && vector < 48) //IRQ interrupt
	{
		outb(0x20, 0x20);
	}
}

//sets up a IDT and all of its entries, maps them to functions 
void idtInit()
{
	memset(idt, 0x0, BOBAOS_TOTAL_INTERRUPTS * sizeof(struct idtEntry));
	memset(handlerList, 0x0, BOBAOS_TOTAL_INTERRUPTS * 0x8);
	
	fillIdt();
	
	initIrqHandler();
	initExceptionHandler();

	idtPtr.offset = (uint64_t)idt;
	idtPtr.size = (sizeof(struct idtEntry) * BOBAOS_TOTAL_INTERRUPTS) - 1;

	loadIdtPtr(&idtPtr);
}

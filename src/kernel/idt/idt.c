#include "idt.h"

#include "config.h"
#include "memory/memory.h"
#include "terminal.h"
#include "io/io.h"

extern uint64_t* idtAddressList[BOBAOS_TOTAL_INTERRUPTS]; //from idt.asm

struct idtEntry idt[BOBAOS_TOTAL_INTERRUPTS];
struct idtPtr idtPtr;

static void printTrapFrame(struct trapFrame* frame)
{
	kprintf("==================/n");
	kprintf("Trap frame: %x/n/n", frame);
	kprintf("RAX: %x  RBX: %x  RCX: %x  RDX: %x/n", frame->rax, frame->rbx, frame->rcx, frame->rdx);
	kprintf("RSI: %x  RDI: %x  RBP: %x/n/n", frame->rsi, frame->rdi, frame->rbp);
	kprintf("R15: %x  R14: %x  R13: %x  R12: %x/n", frame->r15, frame->r14, frame->r13, frame->r12);
	kprintf("R11: %x  R10: %x  R09: %x  R08: %x/n/n", frame->r11, frame->r10, frame->r9, frame->r8);
	kprintf("RIP: %x  CS: %x  EFLAGS: %x/n", frame->rip, frame->cs, frame->eflags);
	kprintf("==================/n");
}

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

void trapHandler(uint16_t vector, struct trapFrame* frame, uint8_t sendAck)
{
	kprintf("Vector No. %u in C-Trapframe/n/n", vector);
	printTrapFrame(frame);

	if(sendAck)
	{
		//outb(0x20, 0x20);
	}
}

//sets up a IDT and all of its entries, maps them to functions and enables interrupts
void idtInit()
{
	memset(idt, 0x0, BOBAOS_TOTAL_INTERRUPTS * sizeof(struct idtEntry));
	memset(idtAddressList, 0x0, BOBAOS_TOTAL_INTERRUPTS * 0x8);

	DEBUG_STRAP_REMOVE_LATER();	
	kprintf("IDT VECTOR 32 ADDRESS: %x/n", idtAddressList[32]);
	
	idtSet(32, idtAddressList[32], IDT_GATE_TYPE_INTERRUPT, 0x0, BOBAOS_KERNEL_SELECTOR_CODE);
	
	idtPtr.offset = (uint64_t)idt;
	idtPtr.size = (sizeof(struct idtEntry) * BOBAOS_TOTAL_INTERRUPTS) - 1;

	loadIdtPtr(&idtPtr);
	enableInterrupts();
}

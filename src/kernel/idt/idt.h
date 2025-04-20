#ifndef IDT_H
#define IDT_H

#include <stdint.h>

#define IDT_DPL_KERNEL 0x0;
#define IDT_DPL_USER 0x3;

enum idtGateType
{
	IDT_GATE_TYPE_INTERRUPT = 0xE,
	IDT_GATE_TYPE_TRAP = 0xB
};

struct idtEntry
{
	uint16_t offset1;
	uint16_t selector;
	uint8_t resered_ist;
	uint8_t attributes;
	uint16_t offset2;
	uint32_t offset3;
	uint32_t reserved;

} __attribute__((packed));

struct idtPtr
{
	uint16_t size;
	uint64_t offset;
} __attribute__((packed));

void idtInit();
void idtSet(uint16_t vector, void* address, enum idtGateType gateType, uint8_t dpl, uint16_t selector);

// Functions in idt.asm
void enableInterrupts();
void disableInterrupts();
void loadIdtPtr(struct idtPtr* ptr);

void DEBUG_STRAP_REMOVE_LATER();

#endif

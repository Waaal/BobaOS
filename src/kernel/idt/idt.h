#ifndef IDT_H
#define IDT_H

#include <stdint.h>

#define IDT_DPL_KERNEL 0x0
#define IDT_DPL_USER 0x3

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

struct trapFrame
{
	uint64_t r15;
	uint64_t r14;	
	uint64_t r13;
	uint64_t r12;
	uint64_t r11;
	uint64_t r10;
	uint64_t r9;
	uint64_t r8;
	uint64_t rbp;
	uint64_t rdi;
	uint64_t rsi;
	uint64_t rdx;
	uint64_t rcx;
	uint64_t rbx;
	uint64_t rax;
} __attribute__((packed));

typedef int8_t (*TRAPHANDLER)(struct trapFrame* frame);

void idtInit();
void idtSet(uint16_t vector, void* address, enum idtGateType gateType, uint8_t dpl, uint16_t selector);
void trapHandler(uint16_t vector, struct trapFrame* frame);
int8_t registerHandler(uint16_t vector, TRAPHANDLER handler);

void printTrapFrame(struct trapFrame* frame);

// Functions in idt.asm
void enableInterrupts();
void disableInterrupts();
void loadIdtPtr(struct idtPtr* ptr);

#endif

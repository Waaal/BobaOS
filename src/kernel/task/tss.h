#ifndef TSS_H
#define TSS_H

#include <stdint.h>

struct tss
{
	uint32_t reserved0;
	uint64_t rsp0;
	uint64_t rsp1;
	uint64_t rsp2;
	uint64_t reserved1;
    uint64_t ist1;
	uint64_t ist2;
	uint64_t ist3;
	uint64_t ist4;
	uint64_t ist5;
	uint64_t ist6;
	uint64_t ist7;
	uint64_t reserved2;
	uint32_t iopb_reserved;
} __attribute__ ((packed));

void initTss();
uint64_t getTssAddress();
void updateTssKernelStack(uint64_t newRsp0);

//functions in tss.asm
void loadTss(uint16_t descriptor);

#endif

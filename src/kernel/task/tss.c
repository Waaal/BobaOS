#include "tss.h"

#include <config.h>
#include <memory/memory.h>

static struct tss tss;

void initTss()
{
    memset(&tss, 0x0, sizeof(struct tss));
    loadTss(BOBAOS_TSS_ENTRY_DESCRIPTOR);
}

uint64_t getTssAddress()
{
    return (uint64_t)&tss;
}

void updateTssKernelStack(uint64_t newRsp0)
{
    tss.rsp0 = newRsp0;
}

#include "mmioEngine.h"

#include <stddef.h>

#include "config.h"
#include "memory.h"
#include "status.h"
#include "paging/paging.h"
#include "kernel.h"

static struct mmioAllocateData allocatedDataStructList[BOBAOS_MMIO_MAX_ALLOC];

static uint64_t allocatedData = 0;
static uint64_t allocatedSize = 0;
static uint64_t currVirtualAddress = BOBAOS_MMIO_VIRTUAL;

void mmioEngineInit()
{
    memset(allocatedDataStructList, 0, sizeof(allocatedDataStructList));
}

void* mmioMap(uint64_t physical, uint64_t size, uint8_t bus, uint8_t device, uint8_t function)
{
    if (allocatedData > BOBAOS_MMIO_MAX_ALLOC) return NULL;
    if (remapPhysicalToVirtualRange((void*)(physical), (void*)currVirtualAddress, size, getKernelPageTable()) < 0) return NULL;

    struct mmioAllocateData* data = &allocatedDataStructList[allocatedData];

    data->busDeviceFunction= (bus << 16) | (device << 8) | function;
    data->physical = physical;
    data->virtual = currVirtualAddress;
    data->size = size;

    allocatedData++;
    allocatedSize += size;
    currVirtualAddress += size;
    return (void*)(currVirtualAddress-size);
}
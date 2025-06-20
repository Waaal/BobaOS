#ifndef MMIOENGINE_H
#define MMIOENGINE_H

#include <stdint.h>

struct mmioAllocateData
{
    uint32_t busDeviceFunction;
    uint64_t physical;
    uint64_t virtual;
    uint64_t size;
    uint8_t flags;
};

void mmioEngineInit();
void* mmioMap(uint64_t physical, uint64_t size, uint8_t bus, uint8_t device, uint8_t function);

#endif

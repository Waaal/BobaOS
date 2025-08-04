#ifndef PARTITIONSTREAM_H
#define PARTITIONSTREAM_H

#include <stdint.h>

#include "disk/disk.h"
#include "disk/stream.h"

struct partitionStream
{
    VOLUMELABEL label;
    uint64_t position;
    uint64_t volumeOffset;
    uint64_t volumeSize;
    struct diskStream* diskStream;
};

struct partitionStream* partitionStreamCreate(VOLUMELABEL label, uint64_t position);
int partitionStreamSeek(struct partitionStream* stream, uint64_t position);
int partitionStreamRead(struct partitionStream* stream, void* out, uint64_t length);
int partitionStreamWrite(struct partitionStream* stream, const void* in, uint64_t length);
void partitionStreamDestroy(struct partitionStream* stream);

#endif

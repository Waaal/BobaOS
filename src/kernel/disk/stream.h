#ifndef _STREAM_H_
#define _STREAM_H_

#include <stdint.h>

#include "disk.h"

struct diskStream
{
    struct disk *disk;
    uint64_t position;
};

struct diskStream* diskStreamCreate(uint8_t driveId, uint64_t position);
void diskStreamSeek(struct diskStream* stream, uint64_t position);
void diskStreamRead(struct diskStream* stream, void* out, uint64_t length);

#endif
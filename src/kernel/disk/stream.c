#include "stream.h"

#include <stddef.h>
#include <memory/memory.h>
#include <memory/kheap/kheap.h>

struct diskStream* diskStreamCreate(uint8_t driveId, uint64_t position)
{
    struct disk* disk = diskGet(driveId);
    if (disk == NULL){return NULL;}

    struct diskStream* stream = kzalloc(sizeof(struct diskStream));
    stream->position = position;
    stream->disk = disk;

    return stream;
}

void diskStreamSeek(struct diskStream* stream, uint64_t position)
{
    if (stream == NULL){return;}
    stream->position = position;
}

void diskStreamRead(struct diskStream* stream, void* out, size_t size)
{
    if (stream == NULL){return;}
    uint64_t currLba = stream->position / 512;
    uint64_t currLbaPos = stream->position % 512;

    uint64_t totalLba = ((currLbaPos + size) / 512) + 1;

    uint8_t* buffer = kzalloc(totalLba * 512);
    stream->disk->driver->read(currLba, totalLba, buffer, stream->disk->driver->private);

    memcpy(out, buffer + currLbaPos, size);
    stream->position += size;

    kzfree(buffer);
}

#include "stream.h"

#include <macros.h>
#include <stddef.h>
#include <memory/memory.h>
#include <memory/kheap/kheap.h>
#include <status.h>

static int checkDiskStream(struct diskStream* stream)
{
    if (stream == NULL){return -EIARG;}
    if (stream->position >= stream->disk->size){return -ENMEM;}
    return  0;
}

struct diskStream* diskStreamCreate(uint8_t driveId, uint64_t position)
{
    struct disk* disk = diskGet(driveId);
    if (disk == NULL){return NULL;}

    struct diskStream* stream = kzalloc(sizeof(struct diskStream));
    stream->position = position;
    stream->disk = disk;

    return stream;
}

int diskStreamSeek(struct diskStream* stream, uint64_t position)
{
    int ret = 0;
    ret = checkDiskStream(stream);
    if (ret < 0){return ret;}

    stream->position = position;

    return ret;
}

int diskStreamRead(struct diskStream* stream, void* out, uint64_t length)
{
    int ret = 0;
    ret = checkDiskStream(stream);
    if (ret < 0){return ret;}

    if (stream->position + length > stream->disk->size){return -ENMEM;}

    uint64_t currLba = stream->position / 512;
    uint64_t currLbaPos = stream->position % 512;

    uint64_t totalLba = ((currLbaPos + length) / 512) + 1;

    uint8_t* buffer = kzalloc(totalLba * 512);
    if (buffer == NULL){return -ENMEM;}

    stream->disk->driver->read(currLba, totalLba, buffer, stream->disk->driver->private);
    stream->position += length;

    memcpy(out, buffer + currLbaPos, length);
    kzfree(buffer);

    return ret;
}

int diskStreamWrite(struct diskStream* stream, const void* in, uint64_t length)
{
    int ret = 0;
    ret = checkDiskStream(stream);
    if (ret < 0){return ret;}

    if (stream->position + length > stream->disk->size){return -ENMEM;}

    uint64_t currLba = stream->position / 512;
    uint64_t currLbaPos = stream->position % 512;

    uint64_t totalLba = ((currLbaPos + length) / 512) + 1;

    if (currLba && totalLba){}
    return -EIO;
}

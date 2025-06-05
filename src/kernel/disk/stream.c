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

    uint64_t totalLba = ((currLbaPos + length) / 512);

    if (((currLbaPos + length) % 512) > 0)
        totalLba++;

    uint8_t* buffer = kzalloc(totalLba * 512);
    if (buffer == NULL){return -ENMEM;}

    ret = stream->disk->driver->read(currLba, totalLba, buffer, stream->disk->driver->private);
    GOTOERROR(ret, out);

    stream->position += length;

    memcpy(out, buffer + currLbaPos, length);

    out:
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
    uint64_t totalLba = ((currLbaPos + length) / 512);

    if (((currLbaPos + length) % 512) > 0)
        totalLba++;

    uint8_t* buffer = kzalloc(totalLba*512);
    RETNULLERROR(buffer, -ENMEM);

    if (currLbaPos > 0)
    {
        //We need to get what was there in the sector
        ret = stream->disk->driver->read(currLba, 1, buffer, stream->disk->driver->private);
        GOTOERROR(ret, out);
    }

    memcpy(buffer + currLbaPos, (void*)in, length);

    if ((currLbaPos + length) % 512 != 0)
    {
        uint8_t buff[512];
        //We need to get what was in last sector and keep it
        ret = stream->disk->driver->read(currLba+totalLba-1, 1, buff, stream->disk->driver->private);
        GOTOERROR(ret, out);

        uint64_t lastBlockOffset = (currLbaPos+length) % 512;

        memcpy(buffer+((totalLba-1)*512)+lastBlockOffset, buff+lastBlockOffset, 512-lastBlockOffset);
    }

    stream->disk->driver->write(currLba, totalLba, buffer, stream->disk->driver->private);
    GOTOERROR(ret, out);

    stream->position += length;

    out:
    kzfree(buffer);
    return ret;
}

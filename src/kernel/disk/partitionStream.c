#include "partitionStream.h"

#include <stddef.h>

#include "status.h"
#include "disk.h"
#include "stream.h"
#include "macros.h"
#include "memory/kheap/kheap.h"

struct partitionStream* partitionStreamCreate(VOLUMELABEL label, uint64_t position)
{
    struct diskPartition* partition = partitionGetByVolumeLabel(label);
    RETNULL(partition);

    struct diskStream* diskStream = diskStreamCreate(partition->disk->id, partition->offsetBytes + position);
    RETNULL(diskStream);
    
    struct partitionStream* stream = kzalloc(sizeof(struct partitionStream));
    RETNULL(stream);
     
    stream->diskStream = diskStream;
    stream->label = label;
    stream->position = position;
    stream->volumeOffset = partition->offsetBytes;
    stream->volumeSize = partition->sizeBytes;
    return stream;
}

int partitionStreamSeek(struct partitionStream* stream, uint64_t position)
{
    if(position < stream->volumeSize)
    {
        diskStreamSeek(stream->diskStream, stream->volumeOffset + position);
        stream->position = position;
        return SUCCESS;
    }
    return -EDISKSPACE; //To VolumeSpace at some point
}

int partitionStreamRead(struct partitionStream* stream, void* out, uint64_t length)
{
    if(stream->position + length > stream->volumeSize)
        return -EDISKSPACE; //Change this to VolumeSpace at some point
    
    int ret = diskStreamRead(stream->diskStream, out, length);
    if(ret == SUCCESS)
        stream->position += length;
    return ret;
}

int partitionStreamWrite(struct partitionStream* stream, const void* in, uint64_t length)
{
    if(stream->position + length > stream->volumeSize)
        return -EDISKSPACE; //Change this to VolumeSpace at some point
    
    int ret = diskStreamWrite(stream->diskStream, in, length);
    if(ret == SUCCESS)
        stream->position += length;
    return ret;
}

void partitionStreamDestroy(struct partitionStream* stream)
{
    kzfree(stream->diskStream);
    kzfree(stream);
}

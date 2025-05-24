#include "fat32.h"

#include <macros.h>
#include <print.h>
#include <stdint.h>
#include <stddef.h>
#include <memory/memory.h>
#include <memory/kheap/kheap.h>
#include <string/string.h>

#include "status.h"
#include "disk/disk.h"
#include "disk/stream.h"

struct fatPrivate
{
    struct masterBootRecord bootRecord;
    uint8_t fatInfo[512];

    uint32_t clusterSize;
    uint32_t fatAddress;
    uint32_t dataClusterStartAddress;
    uint32_t rootDirStartAddress;

    struct diskStream* readStream;
};

static int resolve(struct disk* disk);
static struct fileSystem* attachToDisk(struct disk* disk);
static struct file* openFile(struct pathTracer* tracer, const char* mode, void* private);

struct fileSystem fs =
{
    .name = "FAT32",
    .resolve = resolve,
    .attach = attachToDisk,
    .open = openFile,
    .close = NULL,
    .read = NULL,
    .write = NULL,
    .private = NULL
};

static struct fileSystem* attachToDisk(struct disk* disk)
{
    RETNULL(disk);

    struct fatPrivate* private = kzalloc(sizeof(struct fatPrivate));
    RETNULL(private);

    struct diskStream* stream = diskStreamCreate(disk->id,0);
    RETNULL(stream);

    diskStreamRead(stream, &private->bootRecord, sizeof(struct masterBootRecord));

    private->readStream = stream;
    private->clusterSize = private->bootRecord.bytesPerSector * private->bootRecord.sectorsPerCluster;
    private->fatAddress = private->bootRecord.reservedSectors * private->bootRecord.bytesPerSector;
    private->dataClusterStartAddress = private->bootRecord.reservedSectors * private->bootRecord.bytesPerSector + (private->bootRecord.sectorsPerFat32 * private->bootRecord.bytesPerSector * private->bootRecord.fatCopies);
    private->rootDirStartAddress = (private->bootRecord.reservedSectors * private->bootRecord.bytesPerSector + (private->bootRecord.sectorsPerFat32 * private->bootRecord.bytesPerSector * private->bootRecord.fatCopies)) + ((private->bootRecord.rootDirCluster - 2) * (private->bootRecord.bytesPerSector * private->bootRecord.sectorsPerCluster));

    struct fileSystem* retFs = kzalloc(sizeof(struct fileSystem));
    if (retFs == NULL){ kzfree(private); return NULL; }

    memcpy(retFs, &fs, sizeof(struct fileSystem));
    retFs->private = private;
    return retFs;
}

static int resolve(struct disk* disk)
{
    if (disk == NULL){ return -EIARG}

    unsigned char mbr[512];
    disk->driver->read(0, 1, mbr, disk->driver->private);

    if (strncmp(((struct masterBootRecord*)mbr)->systemIdString, "FAT32   ", 8) == 0)
    {
        return 1;
    }
    return 0;
}

//Get multiple FAT-entries with 1 read operation from disk
static FAT_ENTRY* getMultipleFatEntries(uint32_t start, uint32_t end, struct fatPrivate* private)
{
    FAT_ENTRY* ret = kzalloc(sizeof(FAT_ENTRY) * (end-start));
    RETNULL(ret);

    diskStreamSeek(private->readStream, start * sizeof(FAT_ENTRY) + private->fatAddress);
    diskStreamRead(private->readStream, ret, sizeof(FAT_ENTRY) * (end-start));

    for (uint32_t i = 0; i < (end-start); i++)
        ret[i] = ret[i] & FAT_ENTRY_AND;

    return ret;
}

static FAT_ENTRY getFatEntry(uint32_t entry, struct fatPrivate* private)
{
    FAT_ENTRY ret = 0;
    diskStreamSeek(private->readStream, entry * sizeof(FAT_ENTRY) + private->fatAddress);
    diskStreamRead(private->readStream, &ret, sizeof(FAT_ENTRY));

    return ret & FAT_ENTRY_AND;
}

struct fileSystem* insertIntoFileSystem()
{
    return &fs;
}

static struct file* openFile(struct pathTracer* tracer, const char* mode, void* private)
{
    struct fatPrivate* pr = (struct fatPrivate*)private;

    struct pathTracerPart* part = pathTracerStartTrace(tracer);
    RETNULL(part);

    struct directoryEntry* dirEntry = kzalloc(sizeof(struct directoryEntry));
    RETNULL(dirEntry);

    if (pr->bootRecord.rootDirCluster != 2)
    {
        //Just for now, while testing
        return NULL;
    }

    getFatEntry(0, pr);

    kprintf("\n\nFAT START: %u\n", pr->fatAddress);
    FAT_ENTRY* entries = getMultipleFatEntries(0,5, pr);
    for (uint8_t i = 0; i < 5; i++)
        kprintf("FAT ENTRY%u: %x\n", i, entries[i]);

    struct directoryEntry dirBuffer[(pr->bootRecord.bytesPerSector * pr->bootRecord.sectorsPerCluster) / sizeof(struct directoryEntry)];
    diskStreamSeek(pr->readStream, pr->rootDirStartAddress);
    diskStreamRead(pr->readStream, dirBuffer, 200);

    if (mode){}
    return NULL;
}
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
static struct file* openFile(struct pathTracer* tracer, void* private);
static int readFile(void* ptr, uint64_t size, struct file* file, void* private);
static int writeFile(void* ptr, uint64_t size, struct file* file, void* private);

struct fileSystem fs =
{
    .name = "FAT32",
    .resolve = resolve,
    .attach = attachToDisk,
    .open = openFile,
    .read = readFile,
    .write = writeFile,
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

static uint32_t absoluteAddressToCluster(uint32_t absolute, struct fatPrivate* private)
{
    if (absolute < private->dataClusterStartAddress)
    {
        //Not inside data region
        return 0;
    }
    return 2 + ((private->dataClusterStartAddress - absolute) / private->clusterSize);
}

static uint32_t dataClusterToAbsoluteAddress(uint32_t dataCluster, struct fatPrivate* private)
{
    return (dataCluster-2) * private->clusterSize + private->dataClusterStartAddress;
}

/*
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
*/
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

//Returns all directoryEntries of a directory. Also if the directory is split on multiple clusters
static struct directoryEntry* getDirEntries(uint32_t dataClusterNum, struct fatPrivate* private, uint32_t* outMaxEntries)
{
    uint32_t clusterList[255];
    memset(clusterList, 0, sizeof(clusterList));
    uint32_t clustersTotal = 0;

    FAT_ENTRY fatDir = getFatEntry(dataClusterNum, private);
    while (1)
    {
        clusterList[clustersTotal] = dataClusterNum;
        clustersTotal++;
        dataClusterNum = fatDir;

        if (fatDir != FAT_ENTRY_USED && fatDir != FAT_ENTRY_FREE)
        {
            fatDir = getFatEntry(dataClusterNum, private);
        }
        else
        {
            break;
        }
    }

    if (clustersTotal == 0)
        return NULL;

    struct directoryEntry* ret = kzalloc(private->clusterSize * clustersTotal);
    RETNULL(ret);

    for (uint32_t i = 0; i < clustersTotal; i++)
    {
        diskStreamSeek(private->readStream, dataClusterToAbsoluteAddress(clusterList[i], private));
        diskStreamRead(private->readStream, ret+=(i*private->clusterSize), private->clusterSize);
    }

    *outMaxEntries = (private->clusterSize * clustersTotal) / sizeof(struct directoryEntry);
    return ret;
}

static int compareEntryWithPath(struct directoryEntry* entry, char* path)
{
    int ret = -1;

    char* upperCaseName = toUpperCase(path, strlen(path));

    char* realName = (char*)kzalloc(8);
    char* extension = (char*)kzalloc(3);

    int charPos = findChar(upperCaseName, '.');
    if (charPos >= 0)
    {
        strncpy(extension, upperCaseName+charPos+1, 3);
    }
    strncpy(realName, upperCaseName, charPos);

    ret = strncmp(entry->name, realName, strlen(realName));
    if (charPos >= 0 && ret == 0)
        ret = strncmp(entry->ext, extension, strlen(extension));

    kzfree(realName);
    kzfree(extension);
    kzfree(upperCaseName);
    return ret;
}

static struct directoryEntry* findDirEntry(uint32_t dataClusterNum, char* name, enum dirEntryAttribute attribute, struct fatPrivate* private)
{
    struct directoryEntry* ret = kzalloc(sizeof(struct directoryEntry));
    RETNULL(ret);

    uint32_t maxEntires = 0;
    struct directoryEntry* entries = getDirEntries(dataClusterNum, private, &maxEntires);
    RETNULL(entries);

    uint32_t counter = 0;
    while (entries+counter != NULL && counter < maxEntires)
    {
        if (compareEntryWithPath(entries+counter, name) == 0 && (entries+counter)->attributes == attribute)
        {
            memcpy(ret, entries+counter, sizeof(struct directoryEntry));
            kzfree(entries);
            return ret;
        }
        counter++;
    }

    kzfree(entries);
    kzfree(ret);
    return NULL;
}

static struct fatFile* findFile(struct pathTracer* tracer, struct fatPrivate* private)
{
    struct pathTracerPart* part = pathTracerStartTrace(tracer);
    RETNULL(part);

    uint32_t dataClusterNum = absoluteAddressToCluster(private->rootDirStartAddress, private);
    enum dirEntryAttribute curAttribute = part->type == PATH_TRACER_PART_FILE ? DIR_ENTRY_ATTRIBUTE_ARCHIVE : DIR_ENTRY_ATTRIBUTE_DIRECTORY;
    struct directoryEntry* entry;

    while (1)
    {
        entry = findDirEntry(dataClusterNum, part->pathPart, curAttribute, private);
        part = pathTracerGetNext(part);

        if (part == NULL || entry == NULL)
            break;

        if (curAttribute == DIR_ENTRY_ATTRIBUTE_DIRECTORY)
        {
            dataClusterNum = entry->startClusterHigh << 16 | entry->startClusterLow;
            curAttribute = part->type == PATH_TRACER_PART_FILE ? DIR_ENTRY_ATTRIBUTE_ARCHIVE : DIR_ENTRY_ATTRIBUTE_DIRECTORY;
            kzfree(entry);
        }
        else if (curAttribute == DIR_ENTRY_ATTRIBUTE_ARCHIVE)
        {
            break;
        }
    }

    if (entry != NULL)
    {
        struct fatFile* file = (struct fatFile*)kzalloc(sizeof(struct fatFile));
        RETNULL(file);

        file->fileSize = entry->fileSize;
        file->startCluster = entry->startClusterHigh << 16 | entry->startClusterLow;

        kzfree(entry);
        return file;
    }
    return NULL;
}

static int readFatFile(struct fatFile* fatFile, uint64_t size, uint64_t filePos, void* ptr, struct fatPrivate* private)
{
    uint32_t clustersToRead = size / private->clusterSize;
    uint64_t currentFilePos = filePos % private->clusterSize;
    uint16_t currentToRead = private->clusterSize;
    uint64_t totalRead = 0;

    uint32_t currentCluster = fatFile->startCluster;
    if (filePos >= private->clusterSize)
    {
        uint32_t clustersToSkip = filePos / private->clusterSize;
        for (uint32_t i = 0; i < clustersToSkip; i++)
        {
            //TODO: Error checking
            currentCluster = getFatEntry(currentCluster, private);
        }
    }

    if (size % private->clusterSize != 0)
    {
        clustersToRead++;
    }

    if (size < private->clusterSize)
    {
        currentToRead -= private->clusterSize - (size % private->clusterSize);
    }

    uint32_t i;

    for (i = 0; i < clustersToRead; i++)
    {
        if (currentCluster == FAT_ENTRY_USED || currentCluster ==  FAT_ENTRY_FREE){break;}

        diskStreamSeek(private->readStream,  dataClusterToAbsoluteAddress(currentCluster, private) + currentFilePos);
        diskStreamRead(private->readStream,ptr + totalRead, currentToRead);

        totalRead += currentToRead;
        currentToRead = private->clusterSize;
        currentCluster = getFatEntry(currentCluster, private);
        currentFilePos = 0;

        if (i+2 == clustersToRead)
        {
            //If next cluster is last cluster
            currentToRead = (filePos + size) % private->clusterSize == 0 ? private->clusterSize : size % private->clusterSize;
        }
    }

    if (i != clustersToRead)
    {
        return -EFSYSTEM;
    }
    return SUCCESS;
}

static struct file* openFile(struct pathTracer* tracer, void* private)
{
    struct fatPrivate* pr = (struct fatPrivate*)private;
    struct fatFile* fatFile = findFile(tracer, pr);
    RETNULL(fatFile);

    struct file* file = (struct file*)kzalloc(sizeof(struct file));
    RETNULL(file);

    file->diskId = tracer->diskId;
    file->size = fatFile->fileSize;
    file->position = 0;
    strncpy(file->path, pathTracerGetPathString(tracer), BOBAOS_MAX_PATH_SIZE);

    return file;
}

static int readFile(void* ptr, uint64_t size, struct file* file, void* private)
{
    struct fatPrivate* pr = (struct fatPrivate*)private;
    struct pathTracer* tracer = createPathTracer(file->path);
    RETNULLERROR(tracer, -ENMEM); // If the pathTracer fails its probably because the kernel heap is full

    struct fatFile* fatFile = findFile(tracer,pr);
    destroyPathTracer(tracer);
    RETNULLERROR(fatFile, -ENFOUND);

    return readFatFile(fatFile, size, file->position, ptr, pr);
}

static int writeFile(void* ptr, uint64_t size, struct file* file, void* private)
{
    return -EFSYSTEM;
}
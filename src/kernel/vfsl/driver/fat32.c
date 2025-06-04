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
    uint32_t fatCopyStartAddress;
    uint32_t dataClusterStartAddress;
    uint32_t rootDirStartAddress;

    struct diskStream* readStream;
    struct diskStream* writeStream;
};

static int resolve(struct disk* disk);
static struct fileSystem* attachToDisk(struct disk* disk);
static struct file* openFile(struct pathTracer* tracer, uint8_t create, void* private);
static int readFile(void* ptr, uint64_t size, struct file* file, void* private);
static int writeFile(const void* ptr, uint64_t size, struct file* file, void* private);
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

    struct diskStream* rStream = diskStreamCreate(disk->id,0);
    struct diskStream* wStream = diskStreamCreate(disk->id,0);

    RETNULL(rStream);
    RETNULL(wStream);

    diskStreamRead(rStream, &private->bootRecord, sizeof(struct masterBootRecord));

    private->readStream = rStream;
    private->writeStream = wStream;
    private->clusterSize = private->bootRecord.bytesPerSector * private->bootRecord.sectorsPerCluster;
    private->fatAddress = private->bootRecord.reservedSectors * private->bootRecord.bytesPerSector;
    private->fatCopyStartAddress = private->fatAddress + (private->bootRecord.sectorsPerFat32 * private->bootRecord.bytesPerSector);
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

static uint64_t dataClusterToAbsoluteAddress(uint32_t dataCluster, struct fatPrivate* private)
{
    return (dataCluster-2) * private->clusterSize + private->dataClusterStartAddress;
}

static uint32_t clusterToFATTableAddress(uint32_t cluster, struct fatPrivate* private)
{
    return private->fatAddress + (4*cluster);
}

static uint32_t clusterToFATCopyTableAddress(uint32_t cluster, struct fatPrivate* private)
{
    return private->fatCopyStartAddress + (4*cluster);
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

static uint8_t getCheckSumForLongFileName(char* fileName)
{
    uint8_t sum = 0;
    uint16_t pointPos = findChar(fileName, '.');

    char name[8] = "        ";
    char ext[3] = "   ";

    strncpy(name, fileName, pointPos);
    strncpy(ext, fileName + pointPos + 1, 3);

    char fullNamePadded[11];
    strncpy(fullNamePadded, name, 8);
    strncpy(fullNamePadded + 8, ext, 3);

    char* upperName = toUpperCase(fullNamePadded, 11);
    if (upperName == NULL){return 0;}

    for (int i = 0; i < 11; i++)
    {
        sum = (uint8_t)(((uint8_t)(sum >> 1) | (uint8_t)(sum << 7)) + (uint8_t)upperName[i]) & 0xFF;
    }

    kzfree(upperName);
    return sum;
}

static FAT_ENTRY getFreeFatEntryCluster(struct fatPrivate* private)
{
    uint32_t start = 0;
    uint32_t end = 512;

    while (end < private->bootRecord.totalSectorsBig)
    {
        FAT_ENTRY* entries = getMultipleFatEntries(start, end, private);
        for (uint16_t i = 0; i < 512; i++)
        {
            if (entries[i] == FAT_ENTRY_FREE)
            {
                return start+i;
            }
        }
        start += 512;
        end+= 512;
    }
    return 0;
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

static struct directoryEntry* getDirEntrySingleCluster(uint32_t clusterNum, struct fatPrivate* private, uint32_t* outMaxEntries)
{
    struct directoryEntry* entries = kzalloc(sizeof(struct directoryEntry));
    RETNULL(entries);

    diskStreamSeek(private->readStream, dataClusterToAbsoluteAddress(clusterNum, private));
    diskStreamRead(private->readStream, entries, private->clusterSize);

    *outMaxEntries = private->clusterSize / sizeof(struct directoryEntry);
    return entries;
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
        diskStreamRead(private->readStream, ret+(i*private->clusterSize), private->clusterSize);
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

static int compareEntryWithEntryByFullName(struct directoryEntry* entry1, struct directoryEntry* entry2)
{
    int ret = 0;
    ret = strncmp(entry1->name, entry2->name, 8);
    if (ret == 0)
        ret = strncmp(entry1->ext, entry2->ext, 3);
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

static void writeEntryAtAddress(struct directoryEntry* entry, uint64_t address, struct fatPrivate* private)
{
    diskStreamSeek(private->writeStream, address);
    diskStreamWrite(private->writeStream, entry, sizeof(struct directoryEntry));
}

//This function updates a directoryEntry in a directory
static int updateDirectoryEntry(uint32_t clusterOfDirectory, struct directoryEntry* entryToUpdate, struct directoryEntry* newEntry, struct fatPrivate* private)
{
    int ret = 0;
    FAT_ENTRY nextEntry = clusterOfDirectory;
    uint32_t maxEntriesPerDirectory;

    do
    {
        struct directoryEntry* entries = getDirEntrySingleCluster(nextEntry, private, &maxEntriesPerDirectory);
        for (uint32_t i = 0; i < maxEntriesPerDirectory; i++)
        {
            if ((entries+i)->attributes != 0)
            {
                if (compareEntryWithEntryByFullName(entries+i, entryToUpdate) == 0)
                {
                    writeEntryAtAddress(newEntry, dataClusterToAbsoluteAddress(nextEntry, private) + (i * sizeof(struct directoryEntry)), private);
                    goto out;
                }
            }
        }
        nextEntry = getFatEntry(clusterOfDirectory, private);
    } while (nextEntry != FAT_ENTRY_FREE && nextEntry != FAT_ENTRY_USED);

    ret = -ENFOUND;
    out:
    return ret;
}


//Returns the directory of a file. Also returns the ClusterNumber (*outCluster) of this directory for potential use
static struct directoryEntry* findDirectory(struct pathTracer* tracer, struct fatPrivate* private, uint32_t *outCluster)
{
    if (outCluster != NULL)
        *outCluster = 0;

    struct pathTracerPart* part = pathTracerStartTrace(tracer);
    RETNULL(part);

    uint32_t dataClusterNum = absoluteAddressToCluster(private->rootDirStartAddress, private);
    enum dirEntryAttribute curAttribute = part->type == PATH_TRACER_PART_FILE ? DIR_ENTRY_ATTRIBUTE_ARCHIVE : DIR_ENTRY_ATTRIBUTE_DIRECTORY;
    struct directoryEntry* entry;

    if (curAttribute == DIR_ENTRY_ATTRIBUTE_ARCHIVE)
    {
        //It is a path like 0:file.txt
        //We need to return the rootDir

        struct directoryEntry* entry = kzalloc(sizeof(struct directoryEntry));
        RETNULL(entry);

        entry->startClusterHigh = dataClusterNum >> 16;
        entry->startClusterLow = dataClusterNum & 0xFFFF;
        entry->attributes = DIR_ENTRY_ATTRIBUTE_DIRECTORY;

        if (outCluster != NULL)
            *outCluster = dataClusterNum;
        return entry;
    }

    while (1)
    {
        entry = findDirEntry(dataClusterNum, part->pathPart, curAttribute, private);
        part = pathTracerGetNext(part);

        if (part == NULL || part->type == PATH_TRACER_PART_FILE || entry == NULL)
            break;

        if (curAttribute == DIR_ENTRY_ATTRIBUTE_DIRECTORY)
        {
            if (part->next != NULL && part->next->type == PATH_TRACER_PART_DIRECTORY)
            {
                dataClusterNum = entry->startClusterHigh << 16 | entry->startClusterLow;
                curAttribute = part->type == PATH_TRACER_PART_FILE ? DIR_ENTRY_ATTRIBUTE_ARCHIVE : DIR_ENTRY_ATTRIBUTE_DIRECTORY;
                kzfree(entry);
            }
            else
            {
                break;
            }
        }

        if (curAttribute == DIR_ENTRY_ATTRIBUTE_ARCHIVE)
            break;
    }

    if (entry != NULL && entry->attributes == DIR_ENTRY_ATTRIBUTE_DIRECTORY)
    {
        if (outCluster != NULL)
            *outCluster = dataClusterNum;
        return entry;
    }

    return NULL;
}

static struct fatFile* findFile(struct pathTracer* tracer, struct fatPrivate* private)
{
    struct directoryEntry* entry = findDirectory(tracer, private, NULL);
    if (entry != NULL)
    {
        char* fileName = pathTracerGetFileName(tracer);
        if (fileName == NULL)
        {
            kzfree(entry);
            return NULL;
        }

        uint32_t clusterNum = entry->startClusterHigh << 16 | entry->startClusterLow;
        struct directoryEntry* fileEntry = findDirEntry(clusterNum, fileName, DIR_ENTRY_ATTRIBUTE_ARCHIVE, private);
        kzfree(entry);
        RETNULL(fileEntry);

        struct fatFile* file = (struct fatFile*)kzalloc(sizeof(struct fatFile));
        RETNULL(file);

        file->fileSize = fileEntry->fileSize;
        file->startCluster = fileEntry->startClusterHigh << 16 | fileEntry->startClusterLow;

        kzfree(fileEntry);
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

static void writeFATEntry(uint32_t cluster, uint32_t value, struct fatPrivate* private)
{
    diskStreamSeek(private->writeStream, clusterToFATTableAddress(cluster, private));
    diskStreamWrite(private->writeStream, &value, 4);

    diskStreamSeek(private->writeStream, clusterToFATCopyTableAddress(cluster, private));
    diskStreamWrite(private->writeStream, &value, 4);
}

static uint64_t getFreeDirEntryAddress(struct directoryEntry* entry, struct fatPrivate* private)
{
    FAT_ENTRY fatEntry = getFatEntry(entry->startClusterHigh << 16 | entry->startClusterLow, private);
    FAT_ENTRY oldFatEntry = entry->startClusterHigh << 16 | entry->startClusterLow;

    uint32_t entriesPerCluster = 0;
    struct directoryEntry* entries = getDirEntrySingleCluster((uint32_t)(entry->startClusterHigh << 16 | entry->startClusterLow), private, &entriesPerCluster);

    while (1)
    {
        if (entriesPerCluster < 1)
        {
            if (fatEntry == FAT_ENTRY_USED)
            {
                //We need to alloc more space
                uint32_t newFatEntryCluster = getFreeFatEntryCluster(private);
                if (newFatEntryCluster == 0)
                {
                    //No more space
                    return 0;
                }

                //I think this is wrong lol
                writeFATEntry(oldFatEntry, newFatEntryCluster, private);
                writeFATEntry(newFatEntryCluster, FAT_ENTRY_USED, private);

                //Zero new entries space on disk
                diskStreamSeek(private->writeStream, dataClusterToAbsoluteAddress(newFatEntryCluster, private));
                //Stupid I know
                char temp[private->clusterSize];
                memset(temp, 0, private->clusterSize);
                diskStreamWrite(private->writeStream, temp, private->clusterSize);

                entries = getDirEntrySingleCluster(newFatEntryCluster, private, &entriesPerCluster);
                oldFatEntry = newFatEntryCluster;
                fatEntry = FAT_ENTRY_USED;
            }
            else
            {
                entries = getDirEntrySingleCluster(fatEntry, private, &entriesPerCluster);
                oldFatEntry = fatEntry;
                fatEntry = getFatEntry(fatEntry, private);
            }
        }

        if (entries->attributes == 0) {
            break;
        }

        entries++;
        entriesPerCluster--;
    }

    uint32_t currEntry = (private->clusterSize / sizeof(struct directoryEntry) - entriesPerCluster);
    return dataClusterToAbsoluteAddress(oldFatEntry, private) + (currEntry*sizeof(struct directoryEntry));
}

static struct fatFile* createFileEntryAtAbsoluteAddress(uint64_t address, const char* fileName, struct fatPrivate* private)
{
    uint32_t freeCluster = getFreeFatEntryCluster(private);
    if (freeCluster == 0){return NULL;}

    struct directoryEntry newEntry;
    memset(&newEntry, 0, sizeof(struct directoryEntry));

    int charPos = findChar((char*)fileName, '.');

    char name[8] = "        ";
    char extension[3] = "   ";

    if (charPos >= 0)
    {
        strncpy(name, fileName, charPos);
    }
    strncpy(extension, fileName+charPos+1, 3);

    char* upperName = toUpperCase(name, 8);
    char* upperExt = toUpperCase(extension, 3);

    strncpy(newEntry.name, upperName, 8);
    strncpy(newEntry.ext, upperExt, 3);
    kzfree(upperName);
    kzfree(upperExt);

    newEntry.attributes = DIR_ENTRY_ATTRIBUTE_ARCHIVE;
    newEntry.startClusterHigh = freeCluster >> 16;
    newEntry.startClusterLow = freeCluster & 0xFFFF;
    newEntry.fileSize = 0;

    writeFATEntry(freeCluster, 0x0FFFFFFF, private);
    diskStreamSeek(private->writeStream, address);
    diskStreamWrite(private->writeStream, &newEntry, sizeof(struct directoryEntry));

    struct fatFile* fatFile = kzalloc(sizeof(struct fatFile));
    RETNULL(fatFile);

    fatFile->fileSize = 0;
    fatFile->startCluster = freeCluster;

    return fatFile;
}

static int createLongFileNameEntryAtAddress(uint64_t address, const char* fileName, struct fatPrivate* private)
{
    struct longFileNameEntry longEntry;
    memset(&longEntry, 0, sizeof(struct longFileNameEntry));

    uint8_t fill[13] = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };
    char* ptr = (char*)fileName;

    uint8_t s = 0;

    for (uint8_t i = 0; i < 13; i++)
    {
        if (s || fileName[i] != 0x00)
        {
            if (i < 5)
            {
                longEntry.charsLow[i] = (uint16_t)ptr[i];
            }
            else if (i >= 5 && i < 11)
            {
                longEntry.charsMid[i-5] = (uint16_t)ptr[i];
            }
            else
            {
                longEntry.charsHigh[i-11] = (uint16_t)ptr[i];
            }
        }
        else
        {
            ptr = (char*)fill;
            s = 1;
        }
    }

    longEntry.order = 0x41;
    longEntry.attributes = 0x0F;
    longEntry.checkSum = getCheckSumForLongFileName((char*)fileName);

    diskStreamSeek(private->writeStream, address);
    diskStreamWrite(private->writeStream, &longEntry, sizeof(struct longFileNameEntry));

    return SUCCESS;
}

//Free every FAT linkedListe except the startCluster
static void unlinkFatChain(uint32_t cluster, struct fatPrivate* private)
{
    FAT_ENTRY fatEntry = getFatEntry(cluster, private);
    while (fatEntry != FAT_ENTRY_USED && fatEntry != FAT_ENTRY_FREE)
    {
        FAT_ENTRY temp = fatEntry;
        fatEntry = getFatEntry(fatEntry, private);
        writeFATEntry(temp, FAT_ENTRY_FREE, private);
    }

    writeFATEntry(cluster, FAT_ENTRY_USED, private);
}

static struct fatFile* createFatFileInDir(struct directoryEntry* entry, const char* fileName, struct fatPrivate* private)
{
    uint64_t entryAddress = getFreeDirEntryAddress(entry, private);
    if (entryAddress ==0){return NULL;}
    createLongFileNameEntryAtAddress(entryAddress, fileName, private);

    entryAddress = getFreeDirEntryAddress(entry, private);
    if (entryAddress ==0){return NULL;}
    struct fatFile* file = createFileEntryAtAbsoluteAddress(entryAddress, fileName, private);

    return file;
}

static int writeFatFile(struct fatFile* fatFile, uint64_t size, uint64_t startPos, const void* in, struct fatPrivate* private)
{
    uint32_t startCluster = fatFile->startCluster;
    uint32_t startClusterPos = startPos % private->clusterSize;
    uint32_t clustersToWrite = (startClusterPos + size) / private->clusterSize;

    if ((startClusterPos + size) % private->clusterSize != 0)
        clustersToWrite ++;

    //Skip some clusters
    uint32_t clustersToSkip = startPos / private->clusterSize;
    FAT_ENTRY fatEntry = startCluster;
    FAT_ENTRY oldCluster = startCluster;
    while (clustersToSkip > 0)
    {
        //TODO: Error checking
        oldCluster = fatEntry;
        fatEntry = getFatEntry(fatEntry, private);
        if (fatEntry == FAT_ENTRY_FREE || fatEntry == FAT_ENTRY_USED)
            break;

        clustersToSkip--;
    }

    if (fatEntry != FAT_ENTRY_USED && fatEntry != FAT_ENTRY_FREE)
        oldCluster = fatEntry;

    if (clustersToSkip-1 == 0)
    {
        //If we append we may need to create 1 more cluster to append to
        FAT_ENTRY newCluster = getFreeFatEntryCluster(private);
        if (newCluster == 0)
            return -ENMEM;

        writeFATEntry(oldCluster, newCluster, private);
        writeFATEntry(newCluster, FAT_ENTRY_USED, private);

        startCluster = newCluster;
    }
    else if (clustersToSkip != 0)
    {
        return -ENFOUND;
    }
    else
    {
        startCluster = oldCluster;
    }

    uint16_t currentClusterToWrite = size % private->clusterSize;
    if (currentClusterToWrite == 0)
        currentClusterToWrite = private->clusterSize;

    if (currentClusterToWrite+startClusterPos > private->clusterSize)
    {
        currentClusterToWrite -= (currentClusterToWrite+startClusterPos) % private->clusterSize;
    }

    uint64_t totalWritten = 0;
    uint32_t currCluster = startCluster;
    oldCluster = startCluster;

    unlinkFatChain(currCluster, private);

    for (uint32_t i = 0; i < clustersToWrite; i++)
    {
        diskStreamSeek(private->writeStream, dataClusterToAbsoluteAddress(currCluster, private) + startClusterPos);
        diskStreamWrite(private->writeStream, in + totalWritten, currentClusterToWrite);

        totalWritten += currentClusterToWrite;

        currCluster = getFatEntry(currCluster, private);
        if (currCluster == FAT_ENTRY_USED && i+1 != clustersToWrite)
        {
            FAT_ENTRY newFat = getFreeFatEntryCluster(private);
            if (newFat == 0){return -ENMEM;}

            writeFATEntry(oldCluster, newFat, private);
            writeFATEntry(newFat, FAT_ENTRY_USED, private);

            oldCluster = newFat;
            currCluster = newFat;
        }

        currentClusterToWrite = private->clusterSize;
        if (i+2 == clustersToWrite)
        {
            //See if the last cluster we need to write full cluster or not
            currentClusterToWrite = size - totalWritten;
        }
        startClusterPos = 0;
    }

    return SUCCESS;
}

static struct file* openFile(struct pathTracer* tracer, uint8_t create, void* private)
{
    struct fatPrivate* pr = (struct fatPrivate*)private;

    struct fatFile* fatFile = findFile(tracer, pr);
    if (fatFile == NULL && create)
    {
        struct directoryEntry* entry = findDirectory(tracer, pr, NULL);
        if (entry != NULL)
        {
            const char* fileName = pathTracerGetFileName(tracer);
            if (fileName != NULL)
                fatFile = createFatFileInDir(entry, fileName, pr);

            kzfree(entry);
        }
    }
    RETNULL(fatFile);

    struct file* file = (struct file*)kzalloc(sizeof(struct file));
    RETNULL(file);

    file->diskId = tracer->diskId;
    file->size = fatFile->fileSize;
    file->position = 0;
    strncpy(file->path, pathTracerGetPathString(tracer), BOBAOS_MAX_PATH_SIZE);

    kzfree(fatFile);
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

static int writeFile(const void* ptr, uint64_t size, struct file* file, void* private)
{
    struct fatPrivate* pr = (struct fatPrivate*)private;
    struct pathTracer* tracer = createPathTracer(file->path);
    RETNULLERROR(tracer, -ENMEM);

    struct fatFile* fatFile = findFile(tracer, pr);
    if (fatFile == NULL)
    {
        destroyPathTracer(tracer);
        return -ENFOUND;
    }

    uint64_t startPos = file->position;
    uint8_t append = (file->mode & FILE_MODE_APPEND) > 0 ? 1 : 0;

    if (append)
        startPos = file->size;

    int ret = writeFatFile(fatFile, size, startPos, ptr, pr);
    if (ret == 0)
    {
        uint32_t clusterOfDirectory;
        findDirectory(tracer, pr, &clusterOfDirectory);

        struct directoryEntry* fileEntry = findDirEntry(clusterOfDirectory, pathTracerGetFileName(tracer), DIR_ENTRY_ATTRIBUTE_ARCHIVE, private);
        RETNULLERROR(fileEntry, -ENFOUND); //should never happen

        struct directoryEntry newEntry;
        memcpy(&newEntry, fileEntry, sizeof(struct directoryEntry));
        newEntry.fileSize = (append == 1 ? (file->size + size) : (size + startPos));

        updateDirectoryEntry(clusterOfDirectory, fileEntry, &newEntry, private);
    }

    return ret;
}
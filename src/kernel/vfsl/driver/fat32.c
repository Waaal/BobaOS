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
static struct file* openFile(struct pathTracer* tracer, uint8_t create, void* private, int* oErrCode);
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

    int errCode = 0;
    errCode = diskStreamRead(rStream, &private->bootRecord, sizeof(struct masterBootRecord));
    RETERRORDIFF(errCode, NULL);

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
static FAT_ENTRY* getMultipleFatEntries(uint32_t start, uint32_t end, struct fatPrivate* private, int* oErrCode)
{
    FAT_ENTRY* ret = kzalloc(sizeof(FAT_ENTRY) * (end-start));
    RETNULL(ret);

    *oErrCode = diskStreamSeek(private->readStream, start * sizeof(FAT_ENTRY) + private->fatAddress);
    RETERRORDIFF(*oErrCode, NULL);

    *oErrCode = diskStreamRead(private->readStream, ret, sizeof(FAT_ENTRY) * (end-start));
    RETERRORDIFF(*oErrCode, NULL);

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

static FAT_ENTRY getFreeFatEntryCluster(struct fatPrivate* private, int* oErrCode)
{
    uint32_t start = 0;
    uint32_t end = 512;

    while (end < private->bootRecord.totalSectorsBig)
    {
        FAT_ENTRY* entries = getMultipleFatEntries(start, end, private, oErrCode);
        if (*oErrCode < 0){break;}

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

static FAT_ENTRY getFatEntry(uint32_t entry, struct fatPrivate* private, int* oErrCode)
{
    FAT_ENTRY ret = 0;
    *oErrCode = diskStreamSeek(private->readStream, entry * sizeof(FAT_ENTRY) + private->fatAddress);
    RETERROR(*oErrCode);

    *oErrCode = diskStreamRead(private->readStream, &ret, sizeof(FAT_ENTRY));
    RETERROR(*oErrCode);

    return ret & FAT_ENTRY_AND;
}

struct fileSystem* insertIntoFileSystem()
{
    return &fs;
}

static struct directoryEntry* getDirEntrySingleCluster(uint32_t clusterNum, struct fatPrivate* private, uint32_t* outMaxEntries, int* oErrCode)
{
    struct directoryEntry* entries = kzalloc(sizeof(struct directoryEntry));
    RETNULL(entries);

    *oErrCode = diskStreamSeek(private->readStream, dataClusterToAbsoluteAddress(clusterNum, private));
    RETERRORDIFF(*oErrCode, NULL);

    *oErrCode = diskStreamRead(private->readStream, entries, private->clusterSize);
    RETERRORDIFF(*oErrCode, NULL);

    *outMaxEntries = private->clusterSize / sizeof(struct directoryEntry);
    return entries;
}

//Returns all directoryEntries of a directory. Even if the directory is split on multiple clusters
static struct directoryEntry* getDirEntries(uint32_t dataClusterNum, struct fatPrivate* private, uint32_t* outMaxEntries, int* oErrCode)
{
    uint32_t clusterList[255];
    memset(clusterList, 0, sizeof(clusterList));
    uint32_t clustersTotal = 0;

    FAT_ENTRY fatDir = getFatEntry(dataClusterNum, private, oErrCode);
    if (*oErrCode < 0){return NULL; }

    while (1)
    {
        clusterList[clustersTotal] = dataClusterNum;
        clustersTotal++;
        dataClusterNum = fatDir;

        if (fatDir != FAT_ENTRY_USED && fatDir != FAT_ENTRY_FREE)
        {
            fatDir = getFatEntry(dataClusterNum, private, oErrCode);
            if (*oErrCode < 0){return NULL; }
        }
        else
        {
            break;
        }
    }

    if (clustersTotal == 0)
    {
        *oErrCode = -EFSYSTEM;
        return NULL;
    }

    struct directoryEntry* ret = kzalloc(private->clusterSize * clustersTotal);
    RETNULLSETERROR(ret, -ENMEM, oErrCode);

    for (uint32_t i = 0; i < clustersTotal; i++)
    {
        *oErrCode = diskStreamSeek(private->readStream, dataClusterToAbsoluteAddress(clusterList[i], private));
        if (oErrCode < 0){kzfree(ret); return NULL; }

        *oErrCode = diskStreamRead(private->readStream, ret+(i*private->clusterSize), private->clusterSize);
        if (oErrCode < 0){kzfree(ret); return NULL; }
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

static struct directoryEntry* findDirEntry(uint32_t dataClusterNum, char* name, enum dirEntryAttribute attribute, struct fatPrivate* private, int* oErrCode)
{
    struct directoryEntry* ret = kzalloc(sizeof(struct directoryEntry));
    RETNULLSETERROR(ret, -ENMEM, oErrCode);

    uint32_t maxEntires = 0;
    struct directoryEntry* entries = getDirEntries(dataClusterNum, private, &maxEntires, oErrCode);
    RETNULL(entries); //ErrorCode already set by getDirEntries

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
    *oErrCode = -ENFOUND;
    return NULL;
}

static void writeEntryAtAddress(struct directoryEntry* entry, uint64_t address, struct fatPrivate* private, int* oErrCode)
{
    *oErrCode = diskStreamSeek(private->writeStream, address);
    if (*oErrCode < 0){return;}

    *oErrCode = diskStreamWrite(private->writeStream, entry, sizeof(struct directoryEntry));
}

//This function updates a directoryEntry in a directory
static int updateDirectoryEntry(uint32_t clusterOfDirectory, struct directoryEntry* entryToUpdate, struct directoryEntry* newEntry, struct fatPrivate* private)
{
    int ret = 0;
    FAT_ENTRY nextEntry = clusterOfDirectory;
    uint32_t maxEntriesPerDirectory;

    do
    {
        struct directoryEntry* entries = getDirEntrySingleCluster(nextEntry, private, &maxEntriesPerDirectory, &ret);
        if (ret < 0){ goto out; }
        for (uint32_t i = 0; i < maxEntriesPerDirectory; i++)
        {
            if ((entries+i)->attributes != 0)
            {
                if (compareEntryWithEntryByFullName(entries+i, entryToUpdate) == 0)
                {
                    writeEntryAtAddress(newEntry, dataClusterToAbsoluteAddress(nextEntry, private) + (i * sizeof(struct directoryEntry)), private, &ret);
                    goto out;
                }
            }
        }
        nextEntry = getFatEntry(clusterOfDirectory, private, &ret);
        if (ret < 0){ goto out; }

    } while (nextEntry != FAT_ENTRY_FREE && nextEntry != FAT_ENTRY_USED);

    ret = -ENFOUND;
    out:
    return ret;
}


//Returns the directory of a file. Also returns the ClusterNumber (*outCluster) of this directory for potential use
static struct directoryEntry* findDirectory(struct pathTracer* tracer, struct fatPrivate* private, uint32_t* oCluster, int* oErrCode)
{
    if (oCluster != NULL)
        *oCluster = 0;

    struct pathTracerPart* part = pathTracerStartTrace(tracer);
    RETNULLSETERROR(part, -EIARG, oErrCode);

    uint32_t dataClusterNum = absoluteAddressToCluster(private->rootDirStartAddress, private);
    enum dirEntryAttribute curAttribute = part->type == PATH_TRACER_PART_FILE ? DIR_ENTRY_ATTRIBUTE_ARCHIVE : DIR_ENTRY_ATTRIBUTE_DIRECTORY;
    struct directoryEntry* entry;

    if (curAttribute == DIR_ENTRY_ATTRIBUTE_ARCHIVE)
    {
        //It is a path like 0:file.txt
        //We need to return the rootDir

        struct directoryEntry* entry = kzalloc(sizeof(struct directoryEntry));
        RETNULLSETERROR(entry, -ENMEM, oErrCode);

        entry->startClusterHigh = dataClusterNum >> 16;
        entry->startClusterLow = dataClusterNum & 0xFFFF;
        entry->attributes = DIR_ENTRY_ATTRIBUTE_DIRECTORY;

        if (oCluster != NULL)
            *oCluster = dataClusterNum;
        return entry;
    }

    while (1)
    {
        entry = findDirEntry(dataClusterNum, part->pathPart, curAttribute, private, oErrCode);
        RETNULL(entry); //ErrCode already set by findDirEntry

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
        if (oCluster != NULL)
            *oCluster = dataClusterNum;
        return entry;
    }

    *oErrCode = -ENFOUND;
    return NULL;
}

static struct fatFile* findFile(struct pathTracer* tracer, struct fatPrivate* private, int* oErrCode)
{
    struct directoryEntry* entry = findDirectory(tracer, private, NULL, oErrCode);
    RETNULL(entry);

    char* fileName = pathTracerGetFileName(tracer);
    RETNULLSETERROR(fileName, -EIARG, oErrCode);

    uint32_t clusterNum = entry->startClusterHigh << 16 | entry->startClusterLow;
    struct directoryEntry* fileEntry = findDirEntry(clusterNum, fileName, DIR_ENTRY_ATTRIBUTE_ARCHIVE, private, oErrCode);
    kzfree(entry);
    RETNULL(fileEntry);

    struct fatFile* file = (struct fatFile*)kzalloc(sizeof(struct fatFile));
    RETNULLSETERROR(file, -ENMEM, oErrCode);

    file->fileSize = fileEntry->fileSize;
    file->startCluster = fileEntry->startClusterHigh << 16 | fileEntry->startClusterLow;

    kzfree(fileEntry);
    return file;
}

static int readFatFile(struct fatFile* fatFile, uint64_t size, uint64_t filePos, void* ptr, struct fatPrivate* private)
{
    int ret = 0;

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
            currentCluster = getFatEntry(currentCluster, private, &ret);
            RETERROR(ret);
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

        ret = diskStreamSeek(private->readStream,  dataClusterToAbsoluteAddress(currentCluster, private) + currentFilePos);
        RETERROR(ret);
        ret = diskStreamRead(private->readStream,ptr + totalRead, currentToRead);
        RETERROR(ret);

        totalRead += currentToRead;
        currentToRead = private->clusterSize;
        currentCluster = getFatEntry(currentCluster, private, &ret);
        currentFilePos = 0;

        RETERROR(ret);

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
    return ret;
}

static int writeFATEntry(uint32_t cluster, uint32_t value, struct fatPrivate* private)
{
    int ret = 0;
    ret = diskStreamSeek(private->writeStream, clusterToFATTableAddress(cluster, private));
    RETERROR(ret);
    ret = diskStreamWrite(private->writeStream, &value, 4);
    RETERROR(ret);

    ret = diskStreamSeek(private->writeStream, clusterToFATCopyTableAddress(cluster, private));
    RETERROR(ret);
    ret = diskStreamWrite(private->writeStream, &value, 4);

    return ret;
}

static uint64_t getFreeDirEntryAddress(struct directoryEntry* entry, struct fatPrivate* private, int* oErrCode)
{
    FAT_ENTRY fatEntry = getFatEntry(entry->startClusterHigh << 16 | entry->startClusterLow, private, oErrCode);
    FAT_ENTRY oldFatEntry = entry->startClusterHigh << 16 | entry->startClusterLow;
    if (*oErrCode < 0){return 0;}

    uint32_t entriesPerCluster = 0;
    struct directoryEntry* entries = getDirEntrySingleCluster((uint32_t)(entry->startClusterHigh << 16 | entry->startClusterLow), private, &entriesPerCluster, oErrCode);
    if (*oErrCode < 0){return 0;}

    while (1)
    {
        if (entriesPerCluster < 1)
        {
            if (fatEntry == FAT_ENTRY_USED)
            {
                //We need to alloc more space
                uint32_t newFatEntryCluster = getFreeFatEntryCluster(private, oErrCode);
                if (newFatEntryCluster == 0)
                {
                    //No more space
                    return 0;
                }

                //I think this is wrong lol
                *oErrCode = writeFATEntry(oldFatEntry, newFatEntryCluster, private);
                if (*oErrCode < 0){return 0;}
                *oErrCode = writeFATEntry(newFatEntryCluster, FAT_ENTRY_USED, private);
                if (*oErrCode < 0){return 0;}

                //Zero new entries space on disk
                *oErrCode = diskStreamSeek(private->writeStream, dataClusterToAbsoluteAddress(newFatEntryCluster, private));
                if (*oErrCode < 0){return 0;}
                //Stupid I know
                char temp[private->clusterSize];
                memset(temp, 0, private->clusterSize);
                *oErrCode = diskStreamWrite(private->writeStream, temp, private->clusterSize);
                if (*oErrCode < 0){return 0;}

                entries = getDirEntrySingleCluster(newFatEntryCluster, private, &entriesPerCluster, oErrCode);
                if (*oErrCode < 0){return 0;}

                oldFatEntry = newFatEntryCluster;
                fatEntry = FAT_ENTRY_USED;
            }
            else
            {
                entries = getDirEntrySingleCluster(fatEntry, private, &entriesPerCluster, oErrCode);
                if (*oErrCode < 0){return 0;}

                oldFatEntry = fatEntry;
                fatEntry = getFatEntry(fatEntry, private, oErrCode);
                if (*oErrCode < 0){return 0;}
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

static struct fatFile* createFileEntryAtAbsoluteAddress(uint64_t address, const char* fileName, struct fatPrivate* private, int* oErrCode)
{
    uint32_t freeCluster = getFreeFatEntryCluster(private, oErrCode);
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

    *oErrCode = writeFATEntry(freeCluster, 0x0FFFFFFF, private);
    RETERRORDIFF(*oErrCode, NULL);

    *oErrCode = diskStreamSeek(private->writeStream, address);
    RETERRORDIFF(*oErrCode, NULL);

    *oErrCode = diskStreamWrite(private->writeStream, &newEntry, sizeof(struct directoryEntry));
    RETERRORDIFF(*oErrCode, NULL);

    struct fatFile* fatFile = kzalloc(sizeof(struct fatFile));
    RETNULL(fatFile);

    fatFile->fileSize = 0;
    fatFile->startCluster = freeCluster;

    return fatFile;
}

static int createLongFileNameEntryAtAddress(uint64_t address, const char* fileName, struct fatPrivate* private, int* oErrCode)
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

    *oErrCode = diskStreamSeek(private->writeStream, address);
    RETERROR(*oErrCode);

    *oErrCode = diskStreamWrite(private->writeStream, &longEntry, sizeof(struct longFileNameEntry));
    RETERROR(*oErrCode);

    return SUCCESS;
}

//Free every FAT linkedListe except the startCluster
static int unlinkFatChain(uint32_t cluster, struct fatPrivate* private)
{
    int ret = 0;
    FAT_ENTRY fatEntry = getFatEntry(cluster, private, &ret);
    RETERROR(ret);

    while (fatEntry != FAT_ENTRY_USED && fatEntry != FAT_ENTRY_FREE)
    {
        FAT_ENTRY temp = fatEntry;
        fatEntry = getFatEntry(fatEntry, private, &ret);
        RETERROR(ret);
        ret = writeFATEntry(temp, FAT_ENTRY_FREE, private);
        RETERROR(ret);
    }

    //Todo: Probably should revert the first writeFatEntry if this one fails
    ret = writeFATEntry(cluster, FAT_ENTRY_USED, private);
    return ret;
}

static struct fatFile* createFatFileInDir(struct directoryEntry* entry, const char* fileName, struct fatPrivate* private, int* oErrCode)
{
    uint64_t entryAddress = getFreeDirEntryAddress(entry, private, oErrCode);
    if (entryAddress ==0){return NULL;}
    createLongFileNameEntryAtAddress(entryAddress, fileName, private, oErrCode);
    RETERRORDIFF(*oErrCode, NULL);

    entryAddress = getFreeDirEntryAddress(entry, private, oErrCode);
    if (entryAddress ==0){return NULL;}
    struct fatFile* file = createFileEntryAtAbsoluteAddress(entryAddress, fileName, private, oErrCode);
    RETERRORDIFF(*oErrCode, NULL);

    return file;
}

static int writeFatFile(struct fatFile* fatFile, uint64_t size, uint64_t startPos, const void* in, struct fatPrivate* private)
{
    int ret = 0;

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
        fatEntry = getFatEntry(fatEntry, private, &ret);
        RETERROR(ret);

        if (fatEntry == FAT_ENTRY_FREE || fatEntry == FAT_ENTRY_USED)
            break;

        clustersToSkip--;
    }

    if (fatEntry != FAT_ENTRY_USED && fatEntry != FAT_ENTRY_FREE)
        oldCluster = fatEntry;

    if (clustersToSkip-1 == 0)
    {
        //If we append we may need to create 1 more cluster to append to
        FAT_ENTRY newCluster = getFreeFatEntryCluster(private, &ret);
        RETERROR(ret);

        //Todo: probably should revert if second one fails
        ret = writeFATEntry(oldCluster, newCluster, private);
        RETERROR(ret);
        ret = writeFATEntry(newCluster, FAT_ENTRY_USED, private);
        RETERROR(ret);

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

    ret = unlinkFatChain(currCluster, private);
    RETERROR(ret);

    for (uint32_t i = 0; i < clustersToWrite; i++)
    {
        ret = diskStreamSeek(private->writeStream, dataClusterToAbsoluteAddress(currCluster, private) + startClusterPos);
        RETERROR(ret);
        ret = diskStreamWrite(private->writeStream, in + totalWritten, currentClusterToWrite);
        RETERROR(ret);

        totalWritten += currentClusterToWrite;

        currCluster = getFatEntry(currCluster, private, &ret);
        RETERROR(ret);

        if (currCluster == FAT_ENTRY_USED && i+1 != clustersToWrite)
        {
            FAT_ENTRY newFat = getFreeFatEntryCluster(private, &ret);
            RETERROR(ret);

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

    return ret;
}

static struct file* openFile(struct pathTracer* tracer, uint8_t create, void* private, int* oErrCode)
{
    struct fatPrivate* pr = (struct fatPrivate*)private;

    struct fatFile* fatFile = findFile(tracer, pr, oErrCode);
    if (fatFile == NULL && create && (*oErrCode == 0 || *oErrCode == -4)) // -ENFOUND (-4) is okay in this case
    {
        struct directoryEntry* entry = findDirectory(tracer, pr, NULL, oErrCode);
        if (entry != NULL)
        {
            const char* fileName = pathTracerGetFileName(tracer);
            if (fileName != NULL)
                fatFile = createFatFileInDir(entry, fileName, pr, oErrCode);

            kzfree(entry);
        }
    }
    RETERRORDIFF(*oErrCode, NULL);

    struct file* file = (struct file*)kzalloc(sizeof(struct file));
    RETNULLSETERROR(file, -ENMEM, oErrCode);

    file->diskId = tracer->diskId;
    file->size = fatFile->fileSize;
    file->position = 0;
    strncpy(file->path, pathTracerGetPathString(tracer), BOBAOS_MAX_PATH_SIZE);

    kzfree(fatFile);
    return file;
}

static int readFile(void* ptr, uint64_t size, struct file* file, void* private)
{
    int errCode = 0;
    struct fatPrivate* pr = (struct fatPrivate*)private;
    struct pathTracer* tracer = createPathTracer(file->path, &errCode);
    RETNULLERROR(tracer, errCode); // If the pathTracer fails its probably because the kernel heap is full

    struct fatFile* fatFile = findFile(tracer,pr, &errCode);
    destroyPathTracer(tracer);
    RETNULLERROR(fatFile, errCode);

    return readFatFile(fatFile, size, file->position, ptr, pr);
}

static int writeFile(const void* ptr, uint64_t size, struct file* file, void* private)
{
    int errCode = 0;

    struct fatPrivate* pr = (struct fatPrivate*)private;
    struct pathTracer* tracer = createPathTracer(file->path, &errCode);
    RETNULLERROR(tracer, errCode);

    struct fatFile* fatFile = findFile(tracer, pr, &errCode);
    if (fatFile == NULL)
    {
        destroyPathTracer(tracer);
        return errCode;
    }

    uint64_t startPos = file->position;
    uint8_t append = (file->mode & FILE_MODE_APPEND) > 0 ? 1 : 0;

    if (append)
        startPos = file->size;

    errCode = writeFatFile(fatFile, size, startPos, ptr, pr);
    if (errCode == 0)
    {
        uint32_t clusterOfDirectory;
        findDirectory(tracer, pr, &clusterOfDirectory, &errCode);
        RETERROR(errCode);

        struct directoryEntry* fileEntry = findDirEntry(clusterOfDirectory, pathTracerGetFileName(tracer), DIR_ENTRY_ATTRIBUTE_ARCHIVE, private, &errCode);
        RETNULLERROR(fileEntry, errCode);

        struct directoryEntry newEntry;
        memcpy(&newEntry, fileEntry, sizeof(struct directoryEntry));
        newEntry.fileSize = (append == 1 ? (file->size + size) : (size + startPos));

        errCode = updateDirectoryEntry(clusterOfDirectory, fileEntry, &newEntry, private);
    }

    destroyPathTracer(tracer);
    return errCode;
}
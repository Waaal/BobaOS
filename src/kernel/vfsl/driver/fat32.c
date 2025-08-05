#include "fat32.h"


#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#include <macros.h>
#include <memory/memory.h>
#include <memory/kheap/kheap.h>
#include <string/string.h>
#include <print.h>

#include "config.h"
#include "disk/partitionStream.h"
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
    
    struct partitionStream* readStream;
    struct partitionStream* writeStream;
};

static bool resolve(struct diskPartition* partition);
static struct fileSystem* attachToDisk(struct diskPartition* partition);
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

static struct fileSystem* attachToDisk(struct diskPartition* partition)
{
    RETNULL(partition);

    struct fatPrivate* private = kzalloc(sizeof(struct fatPrivate));
    RETNULL(private);

    struct partitionStream* rStream = partitionStreamCreate(partition->label, 0);
    struct partitionStream* wStream = partitionStreamCreate(partition->label, 0);

    RETNULL(rStream);
    RETNULL(wStream);

    int errCode = 0;
    errCode = partitionStreamRead(rStream, &private->bootRecord, sizeof(struct masterBootRecord));
    GOTOERROR(errCode, error)

    private->readStream = rStream;
    private->writeStream = wStream;
    private->clusterSize = private->bootRecord.bytesPerSector * private->bootRecord.sectorsPerCluster;
    private->fatAddress = private->bootRecord.reservedSectors * private->bootRecord.bytesPerSector;
    private->fatCopyStartAddress = private->fatAddress + (private->bootRecord.sectorsPerFat32 * private->bootRecord.bytesPerSector);
    private->dataClusterStartAddress = private->bootRecord.reservedSectors * private->bootRecord.bytesPerSector + (private->bootRecord.sectorsPerFat32 * private->bootRecord.bytesPerSector * private->bootRecord.fatCopies);
    private->rootDirStartAddress = (private->bootRecord.reservedSectors * private->bootRecord.bytesPerSector + (private->bootRecord.sectorsPerFat32 * private->bootRecord.bytesPerSector * private->bootRecord.fatCopies)) + ((private->bootRecord.rootDirCluster - 2) * (private->bootRecord.bytesPerSector * private->bootRecord.sectorsPerCluster));

    struct fileSystem* retFs = kzalloc(sizeof(struct fileSystem));
    if (retFs == NULL){ goto error; }

    memcpy(retFs, &fs, sizeof(struct fileSystem));
    retFs->private = private;
    return retFs;

error:
    kzfree(private);
    partitionStreamDestroy(rStream);
    partitionStreamDestroy(wStream);
    return NULL;
}

static bool resolve(struct diskPartition* partition)
{
    bool ret = false;

    struct partitionStream* volumeStream = partitionStreamCreate(partition->label, 0);

    unsigned char fatHeader[512];
    partitionStreamRead(volumeStream, fatHeader, 512);

    if (strncmp(((struct masterBootRecord*)fatHeader)->systemIdString, "FAT32   ", 8) == 0)
    {
        ret = true;
    }

    partitionStreamDestroy(volumeStream);
    return ret;
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

    *oErrCode = partitionStreamSeek(private->readStream, start * sizeof(FAT_ENTRY) + private->fatAddress);
    RETERRORDIFF(*oErrCode, NULL);

    *oErrCode = partitionStreamRead(private->readStream, ret, sizeof(FAT_ENTRY) * (end-start));
    RETERRORDIFF(*oErrCode, NULL);

    for (uint32_t i = 0; i < (end-start); i++)
        ret[i] = ret[i] & FAT_ENTRY_AND;

    return ret;
}

static uint8_t getCheckSumFromSEntry(struct directoryEntry* entry)
{
    uint8_t sum = 0;
    
    char upperName[11];
    strncpy(upperName, entry->name, 8);
    strncpy(upperName+8, entry->ext, 3);
    
    for (int i = 0; i < 11; i++)
    {
        sum = (uint8_t)(((uint8_t)(sum >> 1) | (uint8_t)(sum << 7)) + (uint8_t)upperName[i]) & 0xFF;
    }

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
    *oErrCode = partitionStreamSeek(private->readStream, entry * sizeof(FAT_ENTRY) + private->fatAddress);
    RETERROR(*oErrCode);

    *oErrCode = partitionStreamRead(private->readStream, &ret, sizeof(FAT_ENTRY));
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

    *oErrCode = partitionStreamSeek(private->readStream, dataClusterToAbsoluteAddress(clusterNum, private));
    RETERRORDIFF(*oErrCode, NULL);

    *oErrCode = partitionStreamRead(private->readStream, entries, private->clusterSize);
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
        *oErrCode = partitionStreamSeek(private->readStream, dataClusterToAbsoluteAddress(clusterList[i], private));
        if (oErrCode < 0){kzfree(ret); return NULL; }

        *oErrCode = partitionStreamRead(private->readStream, ret+(i*private->clusterSize), private->clusterSize);
        if (oErrCode < 0){kzfree(ret); return NULL; }
    }

    *outMaxEntries = (private->clusterSize * clustersTotal) / sizeof(struct directoryEntry);
    return ret;
}

static struct longFileNameEntry* toLongFileName(const char* name, int orderNumber, uint8_t checkSum, int* oErrCode)
{
    struct longFileNameEntry* longEntry = kzalloc(sizeof(struct longFileNameEntry));
    RETNULLSETERROR(longEntry, -ENMEM, oErrCode);

    longEntry->order = orderNumber;
    longEntry->attributes = 0x0F;
    longEntry->reserved2 = 0x00;
    longEntry->reserved = 0x00;
    longEntry->checkSum = checkSum;

    uint16_t* fields[3];
    uint16_t low[5];
    uint16_t mid[6];
    uint16_t high[2];

    fields[0] = low;
    fields[1] = mid;
    fields[2] = high;

    int len = strlen(name);
    int idx = 0;
    uint8_t terminated = 0;

    for (int group = 0; group < 3; group++) {
        int limit = (group == 0) ? 5 : (group == 1 ? 6 : 2);
        for (int i = 0; i < limit; i++) {
            if (!terminated && idx < len) {
                fields[group][i] = (uint8_t)name[idx++];
            } else if (!terminated) {
                fields[group][i] = 0x0000;  // NULL-Terminator
                terminated = 1;
            } else {
                fields[group][i] = 0xFFFF;  // Padding
            }
        }
    }

    memcpy(longEntry->charsLow, low, 10);
    memcpy(longEntry->charsMid, mid, 12);
    memcpy(longEntry->charsHigh, high, 4);

    return longEntry;
}

/*
static int compareEntryWithPath(struct directoryEntry* entry, char* path)
{
    int ret = 0;

    char realName[8] = {0x20, 0x20, 0x20, 0x20,0x20, 0x20, 0x20, 0x20};
    char extension[3] = {020, 020, 020};

    bool isLong = false;
    ret = toDirEntryName(path, realName, extension, &isLong);
    RETERROR(ret);
    
    uint8_t compLength = 8;
    if(isLong)
        compLength = 6;
    
    ret = strncmp(entry->name, realName, compLength);
    if (ret == 0)
        ret = strncmp(entry->ext, extension, 3);

    return ret;
}

static char* getNameFromEntry(struct directoryEntry* entry, int* oErrCode)
{
    char* name = kzalloc(13);
    RETNULLSETERROR(name, -ENMEM, oErrCode);
    uint8_t i = 0;

    for (i = 0; i < 8; i++)
    {
        if (entry->name[i] == 0x20) break;
        name[i] = entry->name[i];
    }

    if (entry->ext[0] == 0x20 || entry->attributes == DIR_ENTRY_ATTRIBUTE_DIRECTORY) return name;

    name[i++] = '.';
    for (uint8_t j = 0; j < 3; j++)
    {
        if (entry->ext[j] == 0x20) break;
        name[i+j] = entry->ext[j];
    }
    return name;
}
*/
static int compareEntryWithEntryByFullName(struct directoryEntry* entry1, struct directoryEntry* entry2)
{
    int ret = 0;
    ret = strncmp(entry1->name, entry2->name, 8);
    if (ret == 0)
        ret = strncmp(entry1->ext, entry2->ext, 3);
    return ret;
}

static void longFileNameEntriesToName(struct longFileNameEntry** lfnChain, uint8_t entries, char* oName)
{
    uint16_t charCounter = 0;
   
    uint8_t groupOffsets[3] = {1,14,28};
    uint8_t groupLengths[3] = {5,6,2};
    uint16_t* currGroup;

    for(int i = entries-1; i >= 0; i--)
    {
        struct longFileNameEntry* currEntry = lfnChain[i];
        for(uint8_t j = 0; j < 3; j++)
        {
            currGroup = (uint16_t*)(((uint64_t)currEntry)+groupOffsets[j]);
            for(uint8_t l = 0; l < groupLengths[j]; l++)
            {
                if(*(currGroup+l) == 65535)
                    return;
                oName[charCounter] = (char)*(currGroup+l);
                charCounter++; 
            }
        }
    }
}

static int generateShortFileName(struct directoryEntry* currDir, const char* fileName, struct fatPrivate* private, char* oName)
{
    char* upperName = toUpperCase(fileName, strlen(fileName));
    
    int pointPos = findChar((char*)fileName, '.');
    int len = pointPos > 0 ? pointPos : strlen(fileName);

    if(len <= 8)
    {
        char n[13] = {0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, '.', 020, 020, 020};
        
        strncpy(n, upperName, len);
        strncpy(n+9, upperName+len+1, 3);

        strncpy(oName, n, 13);
        kzfree(upperName);
        return SUCCESS;
    }
    
    int low = 9, next = 9, max = 9;
    int errCode = 0;
    bool foundAtLeastOne = false;

    uint32_t entriesCount = 0;
    struct directoryEntry* entries = getDirEntries(currDir->startClusterHigh << 16 | currDir->startClusterLow, private, &entriesCount, &errCode);
    
    for(uint32_t i = 0; i < entriesCount; i++)
    {
        if(entries[i].attributes == DIR_ENTRY_ATTRIBUTE_ARCHIVE)
        {
            if(strncmp(fileName, entries[i].name, 6) == 0 && entries[i].name[6] == '~' && isNumber(entries[i].name[7]))
            {
                uint8_t num = toNumber(entries[i].name[7]);
                if(num < low)
                    low = num;
                else if(num < next)
                    next = num;
                else if(num < max)
                    max = num;

                foundAtLeastOne = true;
            }
        }
    }
    

    int nextFreeNumber = -ENFOUND;
    if(!foundAtLeastOne)
        nextFreeNumber = 1;

    if(next > (1+low))
        nextFreeNumber = low++;
    else if(max > (1+next))
        nextFreeNumber = next++;
     
    if(nextFreeNumber >= 1)
    {
        strncpy(oName, upperName, 6);
        oName[6] = '~';
        oName[7] = '0' + nextFreeNumber;
        oName[8] = '.';

        char t[3] = {020, 020, 020};
        if(pointPos > 0)
        {
            strncpy(t, upperName+pointPos+1, 3);
        }
        strncpy(oName+9, t, 3);
    }

    kzfree(upperName);
    kzfree(entries);
    return nextFreeNumber;
}

static struct directoryEntry* findDirEntry(uint32_t dataClusterNum, char* name, enum dirEntryAttribute attribute, struct fatPrivate* private, int* oErrCode)
{
    struct directoryEntry* ret = kzalloc(sizeof(struct directoryEntry));
    RETNULLSETERROR(ret, -ENMEM, oErrCode);

    uint32_t maxEntires = 0;
    struct directoryEntry* entries = getDirEntries(dataClusterNum, private, &maxEntires, oErrCode);
    if (entries == NULL){goto out;}
    
    struct longFileNameEntry* lfnChain[12] = {0};

    uint8_t curLfnChainCounter = 0;
    uint32_t counter = 0;

    while ((entries+counter)->attributes != 0 && counter < maxEntires)
    {
        if((entries+counter)->attributes == DIR_ENTRY_ATTRIBUTE_LNF)
        {
            lfnChain[curLfnChainCounter] = (struct longFileNameEntry*)(entries+counter);
            curLfnChainCounter++;
        }
        else
        {
            if(curLfnChainCounter > 0)
            {
                char fullName[BOBAOS_MAX_PATH_SIZE];
                longFileNameEntriesToName(lfnChain, curLfnChainCounter, fullName);

                if(strncmp(fullName, name, BOBAOS_MAX_PATH_SIZE) == 0)
                {
                    uint8_t checkSum = getCheckSumFromSEntry(entries+counter);
                    uint8_t lfnCheckSum = ((struct longFileNameEntry*)(entries+counter-1))->checkSum;
                    if((entries+counter)->attributes == attribute && checkSum == lfnCheckSum)
                    {
                        goto found;
                    }
                }
            }

            //TODO: If there isnt a LFN entry then the SFN entry doesnt have one.
            //      Check this case 

            curLfnChainCounter = 0;
        }
        counter++;
    }
    
    if(lfnChain[0] == NULL){}

    out:
    *oErrCode = -ENFOUND;
    kzfree(entries);
    kzfree(ret);
    return NULL;

    found:
    memcpy(ret, entries+counter, sizeof(struct directoryEntry));
    kzfree(entries);
    return ret;
}

static void writeEntryAtAddress(struct directoryEntry* entry, uint64_t address, struct fatPrivate* private, int* oErrCode)
{
    *oErrCode = partitionStreamSeek(private->writeStream, address);
    if (*oErrCode < 0){return;}

    *oErrCode = partitionStreamWrite(private->writeStream, entry, sizeof(struct directoryEntry));
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
        //It is a path like C:/file.txt
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

        ret = partitionStreamSeek(private->readStream,  dataClusterToAbsoluteAddress(currentCluster, private) + currentFilePos);
        RETERROR(ret);
        ret = partitionStreamRead(private->readStream,ptr + totalRead, currentToRead);
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
    ret = partitionStreamSeek(private->writeStream, clusterToFATTableAddress(cluster, private));
    RETERROR(ret);
    ret = partitionStreamWrite(private->writeStream, &value, 4);
    RETERROR(ret);

    ret = partitionStreamSeek(private->writeStream, clusterToFATCopyTableAddress(cluster, private));
    RETERROR(ret);
    ret = partitionStreamWrite(private->writeStream, &value, 4);

    return ret;
}

static int makeDirBigger(uint64_t endCluster, struct fatPrivate* private)
{
    int errCode = 0;
    uint32_t newFatEntryCluster = getFreeFatEntryCluster(private, &errCode);
    
    RETERROR(errCode);
    if (newFatEntryCluster == 0)
        return -EDISKSPACE;

    errCode = writeFATEntry(endCluster, newFatEntryCluster, private);
    RETERROR(errCode);
    errCode = writeFATEntry(newFatEntryCluster, FAT_ENTRY_USED, private);
    RETERROR(errCode);

    //Zero new entries space on partition
    errCode = partitionStreamSeek(private->writeStream, dataClusterToAbsoluteAddress(newFatEntryCluster, private));
    RETERROR(errCode);

    char temp[private->clusterSize];
    memset(temp, 0, private->clusterSize);
    errCode = partitionStreamWrite(private->writeStream, temp, private->clusterSize);
    RETERROR(errCode);

    return newFatEntryCluster;
}

static uint64_t* getFreeDirEntriesAddresses(struct directoryEntry* startEntry, uint8_t count, struct fatPrivate* private, int* oErrCode)
{
    uint64_t* dirEntryAddressList = kzalloc(count);
    RETNULLSETERROR(dirEntryAddressList, -ENMEM, oErrCode);
    
    uint64_t oldCluster = startEntry->startClusterHigh << 16 | startEntry->startClusterLow; 
    FAT_ENTRY fatEntry = getFatEntry(oldCluster, private, oErrCode);
    if(*oErrCode < 0){return NULL;}
    
    uint32_t maxEntriesPerCluster = 0;
    struct directoryEntry* entries = getDirEntrySingleCluster((uint32_t)(startEntry->startClusterHigh << 16 | startEntry->startClusterLow), private, &maxEntriesPerCluster, oErrCode);
    
    uint32_t entriesCount = 0;
    uint8_t freeEntriesFoundCount = 0;
    while(1)
    {
        if(entriesCount == maxEntriesPerCluster)
        {
            //Get next cluster or create new cluster

            if(fatEntry == FAT_ENTRY_USED)
            {
                uint64_t temp = oldCluster;
                oldCluster = fatEntry;
                fatEntry = makeDirBigger(temp, private);
            }
            
            kzfree(entries);
            entries = getDirEntrySingleCluster(fatEntry, private, &maxEntriesPerCluster, oErrCode);
            
            oldCluster = fatEntry;
            fatEntry = getFatEntry(fatEntry, private, oErrCode);
            GOTOERROR(*oErrCode, error)
            
            entriesCount = 0;
            continue;
        }
        else
        {
            if((entries+entriesCount)->attributes == 0)
            {
                dirEntryAddressList[freeEntriesFoundCount] = dataClusterToAbsoluteAddress(oldCluster, private) + (sizeof(struct directoryEntry) * entriesCount);
                freeEntriesFoundCount++;

                if(freeEntriesFoundCount == count)
                    break;
            }
            else
            {
                freeEntriesFoundCount = 0;
            }

            entriesCount++;
        }

    }

    return dirEntryAddressList;

error:
    kzfree(entries);
    return NULL;
}

static struct fatFile* createFileEntryAtAbsoluteAddress(uint64_t address, const char* fileName, struct fatPrivate* private, int* oErrCode)
{
    uint32_t freeCluster = getFreeFatEntryCluster(private, oErrCode);
    if (freeCluster == 0){return NULL;}

    struct directoryEntry newEntry;
    memset(&newEntry, 0, sizeof(struct directoryEntry));
    strncpy(newEntry.name, fileName, 8);
    strncpy(newEntry.ext, fileName+9, 3);

    newEntry.attributes = DIR_ENTRY_ATTRIBUTE_ARCHIVE;
    newEntry.startClusterHigh = freeCluster >> 16;
    newEntry.startClusterLow = freeCluster & 0xFFFF;
    newEntry.fileSize = 0;

    *oErrCode = writeFATEntry(freeCluster, 0x0FFFFFFF, private);
    RETERRORDIFF(*oErrCode, NULL);

    *oErrCode = partitionStreamSeek(private->writeStream, address);
    RETERRORDIFF(*oErrCode, NULL);

    *oErrCode = partitionStreamWrite(private->writeStream, &newEntry, sizeof(struct directoryEntry));
    RETERRORDIFF(*oErrCode, NULL);

    struct fatFile* fatFile = kzalloc(sizeof(struct fatFile));
    RETNULL(fatFile);

    fatFile->fileSize = 0;
    fatFile->startCluster = freeCluster;

    return fatFile;
}

static int createLongFileNameEntryAtAddress(uint64_t* address, const char* fileName, uint8_t checkSum, struct fatPrivate* private, int* oErrCode)
{
    uint16_t fileNameLength = strlen(fileName);
    uint8_t entriesNeeded = fileNameLength / 13;
    if(fileNameLength % 13 > 0)
        entriesNeeded++;
    
    int count = 0;
    for(int i = entriesNeeded-1; i >= 0; i--)
    {
        struct longFileNameEntry* longEntry =  toLongFileName(fileName+i*13, i+1, checkSum,oErrCode);
        RETERROR(*oErrCode);
        
        if(i == entriesNeeded-1)
            longEntry->order |= LFN_ENTRY_LAST_FLAG;

        *oErrCode = partitionStreamSeek(private->writeStream, address[count]);
        RETERROR(*oErrCode);

        *oErrCode = partitionStreamWrite(private->writeStream, longEntry, sizeof(struct longFileNameEntry));
        RETERROR(*oErrCode);
        
        count++;

        kzfree(longEntry);
    }

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
    uint32_t fileLength = strlen(fileName);
    
    uint8_t entriesNeeded = fileLength / 13;
    if(fileLength % 13 > 0)
        entriesNeeded++; 
    entriesNeeded++; //SFN-entry
    
    uint64_t* addressList = getFreeDirEntriesAddresses(entry, entriesNeeded, private, oErrCode); 
    struct fatFile* file = NULL;
    GOTOERROR(*oErrCode, out);
    
    char shortFileName[BOBAOS_MAX_PATH_SIZE] = {0};
    generateShortFileName(entry, fileName, private, shortFileName);
    
    struct directoryEntry tempEntry;
    strncpy(tempEntry.name, shortFileName, 8);
    strncpy(tempEntry.ext, shortFileName+9, 3);
    uint8_t checkSum = getCheckSumFromSEntry(&tempEntry);

    createLongFileNameEntryAtAddress(addressList, fileName, checkSum, private, oErrCode);
    GOTOERROR(*oErrCode, out);
    
    file = createFileEntryAtAbsoluteAddress(addressList[entriesNeeded-1], shortFileName, private, oErrCode); 
    

out:
    if(addressList != NULL)
        kzfree(addressList);
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
        ret = partitionStreamSeek(private->writeStream, dataClusterToAbsoluteAddress(currCluster, private) + startClusterPos);
        RETERROR(ret);
        ret = partitionStreamWrite(private->writeStream, in + totalWritten, currentClusterToWrite);
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
    if (fatFile == NULL && create && (*oErrCode == 0 || *oErrCode == -ENFOUND))// Not found is okay in this case
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

    file->label = tracer->label;
    file->size = fatFile->fileSize;
    file->position = 0;
    strncpy(file->path, pathTracerGetPathString(tracer), BOBAOS_MAX_PATH_SIZE);
    strncpy(file->name, pathTracerGetFileName(tracer), BOBAOS_MAX_PATH_SIZE);

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

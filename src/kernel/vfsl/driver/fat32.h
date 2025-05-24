#ifndef FAT32_H
#define FAT32_H

#include <stdint.h>

#include "vfsl/virtualFilesystemLayer.h"

#define FAT_ENTRY_AND 0x0FFFFFFF;

typedef uint32_t FAT_ENTRY;

struct masterBootRecord
{
    uint8_t reserved[3];
    uint8_t OEMIdentifier[8];
    uint16_t bytesPerSector;
    uint8_t sectorsPerCluster;
    uint16_t reservedSectors;
    uint8_t fatCopies;
    uint16_t rootEntries;
    uint16_t totalSectors;
    uint8_t mediaDescriptor;
    uint16_t sectorsPerFat12_16;
    uint16_t sectorsPerTrack;
    uint16_t heads;
    uint32_t hiddenSectors;
    uint32_t totalSectorsBig;
    uint32_t sectorsPerFat32;
    uint16_t flags;
    uint16_t fatVersion;
    uint32_t rootDirCluster;
    uint16_t infoCluster;
    uint16_t backupBootCluster;
    uint8_t reserved2[12];
    uint8_t driveNumber;
    uint8_t reserved3;
    uint8_t signature;
    uint32_t volumeId;
    char volumeLabel[11];
    char systemIdString[8];
} __attribute__ ((packed));

struct directoryEntry
{
    char name[8];
    char ext[3];
    uint8_t attributes;
    uint8_t reserved;
    uint8_t creationTimeM;
    uint16_t creationHMS;
    uint16_t creationTimeYMD;
    uint16_t lastAccessTimeHMS;
    uint16_t startClusterHigh;
    uint16_t modifiedTimeHMS;
    uint16_t modifiedTimeYMD;
    uint16_t startClusterLow;
    uint32_t fileSize;
} __attribute__ ((packed));

struct fileSystem* insertIntoFileSystem();

#endif

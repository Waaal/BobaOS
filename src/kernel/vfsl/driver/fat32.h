#ifndef FAT32_H
#define FAT32_H

#include <stdint.h>

#include "vfsl/virtualFilesystemLayer.h"

#define FAT_ENTRY_AND 0x0FFFFFFF

#define FAT_ENTRY_FREE 0x0
#define FAT_ENTRY_USED 0xFFFFFFF

enum dirEntryAttribute
{
    DIR_ENTRY_ATTRIBUTE_READ_ONLY = 0x1,
    DIR_ENTRY_ATTRIBUTE_HIDDEN = 0x2,
    DIR_ENTRY_ATTRIBUTE_SYSTEM_FILE = 0x4,
    DIR_ENTRY_ATTRIBUTE_VOLUME_ID = 0x8,
    DIR_ENTRY_ATTRIBUTE_DIRECTORY = 0x10,
    DIR_ENTRY_ATTRIBUTE_ARCHIVE = 0x20,
    DIR_ENTRY_ATTRIBUTE_DEVICE = 0x40,
    DIR_ENTRY_ATTRIBUTE_RESERVED = 0x80,
    DIR_ENTRY_ATTRIBUTE_LNF = 0xF
};

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

struct longFileNameEntry
{
    uint8_t order;
    uint16_t charsLow[5];
    uint8_t attributes;
    uint8_t reserved;
    uint8_t checkSum;
    uint16_t charsMid[6];
    uint16_t reserved2;
    uint16_t charsHigh[2];
} __attribute__ ((packed));

struct fatFile
{
    uint32_t startCluster;
    uint32_t fileSize;
};

struct fileSystem* insertIntoFileSystem();

#endif

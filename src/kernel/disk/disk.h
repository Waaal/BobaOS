#ifndef DISK_H
#define DISK_H

#include <stdint.h>

#include "config.h"
#include "diskDriver.h"

#define LABEL_ASCII_OFFSET 67 //C

#define MBR_ACTIVE_FLAG 0x80
#define MBR_PARITION_TABLES_OFFSET 0x1BE

enum diskType
{
	DISK_TYPE_PHYSICAL = 0x1
};

enum diskPartitionsType
{
    DISK_PARTITION_TYPE_MBR,
    DISK_PARTITION_TYPE_GPT, //Not supported yet
    DISK_PARTITION_TYPE_SUPPER_FLOPPY // Not supported yet
};

typedef char VOLUMELABEL;

struct mbrPartitionTable
{
    uint8_t bootable;
    uint8_t startingHead;
    uint16_t startingSectorCylinder;
    uint8_t systemId;
    uint8_t endingHead;
    uint16_t endingSectorCylinder;
    uint32_t partitionStart;
    uint32_t totalSectorsInPartition;
} __attribute__((packed));

struct diskPartition
{
    uint8_t id;
    VOLUMELABEL label;
    uint64_t offsetBytes;
    uint64_t sizeBytes;
    struct disk* disk;
    struct fileSystem* fileSystem;
};

struct fileSystem;
struct disk
{
	uint8_t id;
	char name[64];
	uint64_t sizeBytes;
	enum diskType type;
    uint8_t partitionCount;
    enum diskPartitionsType partitionType;
    struct diskPartition partition[4];
    struct diskDriver* driver;
	struct fileSystem* fileSystem;
};

int diskInit();
struct disk* diskGet(uint8_t id);
struct disk** diskGetAll();
struct diskPartition* partitionGet(uint8_t diskId, uint8_t partitionId);
struct diskPartition* partitionGetByVolumeLabel(VOLUMELABEL label);

#endif

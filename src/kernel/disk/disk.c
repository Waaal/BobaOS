#include "disk.h"

#include <stddef.h>
#include <stdbool.h>

#include <hardware/pci/pci.h>
#include <memory/memory.h>

#include "macros.h"
#include "memory/kheap/kheap.h"
#include "print.h"
#include "config.h"
#include "status.h"
#include "string/string.h"
#include "stream.h"

struct disk* diskList[BOBAOS_MAX_DISKS];
struct diskPartition* partitionList[BOBAOS_MAX_DISKS * BOBAOS_MAX_PARTITIONS_PER_DISK];

uint16_t currentDisk = 0;
uint16_t currentPartition = 0;
char currentPartitionLabel = BOBAOS_DEFAULT_VOLUME_LABEL;

static int insertDisk(struct disk* disk)
{
	if(currentDisk == BOBAOS_MAX_DISKS)
	{
		return -ENMEM;
	}
	diskList[currentDisk] = disk;
	currentDisk++;

	return 0;
}

//Returns number of partitions
static int checkMbrPartition(struct disk* disk, uint8_t* sectorBuffer)
{
    sectorBuffer += MBR_PARITION_TABLES_OFFSET;
    uint8_t diskParitionCount = 0;

    for(uint8_t i = 0; i < 4; i++)
    {
        struct mbrPartitionTable* tables = (struct mbrPartitionTable*)sectorBuffer;
        
        if(tables[i].bootable == MBR_ACTIVE_FLAG)
        {
            if(currentPartition >= BOBAOS_MAX_DISKS * BOBAOS_MAX_PARTITIONS_PER_DISK)
            {
                print("  Reached max limit on supported partitons by OS\n");
                return diskParitionCount;
            }

            //We found a partition
            disk->partition[diskParitionCount].id = diskParitionCount; 
            disk->partition[diskParitionCount].fileSystem = NULL; 
            disk->partition[diskParitionCount].offsetBytes = tables[i].partitionStart * 512; 
            disk->partition[diskParitionCount].label = currentPartitionLabel; 
            disk->partition[diskParitionCount].disk = disk; 
            disk->partition[diskParitionCount].sizeBytes = tables[i].totalSectorsInPartition * 512;

            diskParitionCount++;
            currentPartitionLabel++;

            partitionList[currentPartition] = disk->partition;
            currentPartition++;
        }
        sectorBuffer+= sizeof(struct mbrPartitionTable);
    }
    return diskParitionCount;
}

static int checkDiskPartition(struct disk* disk)
{
    uint8_t buff[512];

    RETERROR(disk->driver->read(0, 1, buff, disk->driver->private));
    
    int mbrPartitions = checkMbrPartition(disk, buff);
    if(mbrPartitions)
    {
        kprintf("  Found %u partition(s) on disk\n", mbrPartitions);
        disk->partitionCount = mbrPartitions;
        goto out;
    }
    kprintf("  No MBR partitons found on disk\n");
    
    //Scan for gpt
    
    //Then check for supperfloppy
out:
    return SUCCESS;
}

//See if a disk is connected to legacy ports
static void scanLegacyPorts()
{
	//Check if we have a driver for ATAPIO legacy ports
	struct diskDriver* driver = getDriver(DISK_DRIVER_TYPE_ATA_LEGACY);
	if(driver != NULL)
	{
		struct disk* tempDiskList[BOBAOS_MAX_DISKS];
		int diskFoundCount = 0;

		driver->scanForDisk(tempDiskList, &diskFoundCount, currentDisk);
		for (int i = 0; i < diskFoundCount; i++)
		{
			struct diskInfo info = tempDiskList[i]->driver->getInfo(tempDiskList[i]->driver->private);
			//TODO: retunr error
			tempDiskList[i]->sizeBytes = info.size;
			strncpy(tempDiskList[i]->name, info.name, 64);
            
            checkDiskPartition(tempDiskList[i]);
			if (insertDisk(tempDiskList[i])< 0)
				return;
		}
	}
}

static void scanSata()
{
	struct diskDriver* driver = getDriver(DISK_DRIVER_TYPE_AHCI);
	if(driver != NULL) {
		struct disk* tempDiskList[BOBAOS_MAX_DISKS];
		int diskFoundCount = 0;

		driver->scanForDisk(tempDiskList, &diskFoundCount, currentDisk);
		for (int i = 0; i < diskFoundCount; i++)
		{
			struct diskInfo info = tempDiskList[i]->driver->getInfo(tempDiskList[i]->driver->private);
			//TODO: retunr error
			tempDiskList[i]->sizeBytes = info.size;
			strncpy(tempDiskList[i]->name, info.name, 64);

            checkDiskPartition(tempDiskList[i]);
			if (insertDisk(tempDiskList[i])< 0)
				return;
		}
	}
}

//This function trys to find all available disks on the system
static void scanDisks()
{
	scanLegacyPorts();
	scanSata();
}

//This funktion tries to find the partition of the System and give it the volume label 'C' (also place it at the start of the partitionList)
static bool findKernelPartition()
{

    struct diskStream* stream = diskStreamCreate(0, 0);
    if(stream == NULL) return false;

    char buff[12] = {0};
    const char* bobaOSSignature = "BOBAOS BOOT";

    for(uint8_t i = 0; i < diskList[0]->partitionCount; i++)
    {
        //We know the filesystem needs to be FAT32 and the VolumeIdString (offset 0x47) is 'BOBAOS BOOT' 
        diskStreamSeek(stream, diskList[0]->partition[i].offsetBytes + 0x47);
        if(diskStreamRead(stream, buff, 11) < 0)
        {
            //TODO error
        }

        if(strcmp(bobaOSSignature, buff) == 0)
        {
            if(diskList[0]->partition[i].label == BOBAOS_DEFAULT_VOLUME_LABEL)
            {
                //do nothing
                return true;
            }

            //Volume C needs to be at the start of the partitionList
            struct diskPartition* tempDiskPartitionPtr = partitionList[0];
            uint16_t currKernelPartitionIndex = diskList[0]->partition[i].label - LABEL_ASCII_OFFSET;

            partitionList[0] = &diskList[0]->partition[i];
            partitionList[currKernelPartitionIndex] = tempDiskPartitionPtr;
        
            partitionList[currKernelPartitionIndex]->label = partitionList[0]->label;
            partitionList[0]->label = BOBAOS_DEFAULT_VOLUME_LABEL;

            return true;
        }        
    } 
    return false;
}

//This function tries to find the disk the kernel boots from and give ot the id 0 (start of the diskList).
static bool findKernelDisk()
{
	for (uint16_t i = 0; i < currentDisk; i++)
	{
		if (diskList[i] == NULL){break;}

		char buff[512];
		char* bobaOsSignature = "BOBA";
		diskList[i]->driver->read(0, 1, &buff, diskList[i]->driver->private);
		if (memcmp(buff+442, bobaOsSignature, 4) == 0)
		{
			//found kernel disk
			if (i != 0)
			{
				struct disk* tempDisk = diskList[0];
				diskList[0] = diskList[i];
				diskList[i] = tempDisk;

				diskList[0]->id = 0;
				diskList[i]->id = i;
			}
			return true;
		}
	}
	return false;
}

struct disk* diskGet(uint8_t id)
{
	if (id >= currentDisk)
	{
		return NULL;
	}
	return diskList[id];
}

struct disk** diskGetAll()
{
	return diskList;
}

struct diskPartition* partitionGet(uint8_t diskId, uint8_t partitionId)
{
    if(diskId >= currentDisk || partitionId >= BOBAOS_MAX_PARTITIONS_PER_DISK)
        return NULL;
    return &diskList[diskId]->partition[partitionId];
}

struct diskPartition* partitionGetByVolumeLabel(VOLUMELABEL label)
{
    if(label-LABEL_ASCII_OFFSET >= currentPartition)
        return NULL;
    return partitionList[LABEL_ASCII_OFFSET-label];
}

struct diskPartition** partitionGetAll()
{
    return partitionList;
}

int diskInit()
{
	memset(diskList, 0x0, sizeof(struct disk*) * BOBAOS_MAX_DISKS);

	scanDisks();

	if (findKernelDisk() && findKernelPartition())
	{
		return SUCCESS;
	}
	else
	{
		return -ENFOUND;
	}
}

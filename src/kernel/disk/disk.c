#include "disk.h"

#include <stddef.h>
#include <hardware/pci/pci.h>
#include <memory/memory.h>
#include <vfsl/driver/fat32.h>

#include "config.h"
#include "status.h"
#include "disk/driver/ataPio.h"
#include "memory/kheap/kheap.h"
#include "string/string.h"
#include "print.h"

struct disk* diskList[BOBAOS_MAX_DISKS];
uint16_t currentDisk = 0;

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
		for (uint16_t i = 0; i < diskFoundCount; i++)
		{
			struct diskInfo info = tempDiskList[i]->driver->getInfo(tempDiskList[i]->driver->private);
			//TODO: retunr error
			tempDiskList[i]->size = info.size;
			strncpy(tempDiskList[i]->name, info.name, 64);

			if (insertDisk(tempDiskList[i])< 0)
				return;
		}
	}
}

//This function trys to find all available disks on the system
static void scanDisks()
{
	scanLegacyPorts();
}

//This function tries to find the disk the kernel boots from and give ot the id 0 (start of the diskList).
static int findKernelDisk()
{
	int ret = 0;
	for (uint16_t i = 0; i < currentDisk; i++)
	{
		if (diskList[i] == NULL){break;}

		char buff[512];
		char* bobaOsSignature = "BOBAV";
		diskList[i]->driver->read(0, 1, &buff, diskList[i]->driver->private);
		if (memcmp(buff+3, bobaOsSignature, 5) == 0)
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
			ret = 1;
		}
	}
	return ret;
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

int diskInit()
{
	memset(diskList, 0x0, sizeof(struct disk*) * BOBAOS_MAX_DISKS);

	scanDisks();
	if (findKernelDisk() == 1)
	{
		return 0;
	}
	else
	{
		return -ENFOUND;
	}
}

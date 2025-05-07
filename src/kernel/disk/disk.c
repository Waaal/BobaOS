#include "disk.h"

#include <stddef.h>

#include "config.h"
#include "status.h"
#include "disk/driver/ataPio.h"
#include "memory/kheap/kheap.h"

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
		//TODO: probe all legacy ports lol
		int diskAvailable = ataPioProbeLegacyPorts(0x1F0);
		if(diskAvailable > 0)
		{
			//We found a disk. YAYYYYYYYYYYY :3
			struct disk* disk = (struct disk*)kzalloc(sizeof(struct disk));
			
			disk->id = currentDisk;
			disk->type = DISK_TYPE_PHYSICAL;
			disk->driver = driver;
			if(ataPioAttach(disk, 0x1F0, 0xA0) < 0)
			{
				kzfree(disk);
			}
			else
			{
				insertDisk(disk);
			}
		}
	}
}

//This function trys to find all available disks on the system
static void scanDisks()
{
	scanLegacyPorts();
	//TODO: scan pci bus etc
}

int diskInit()
{
	scanDisks();
	return 0;
}

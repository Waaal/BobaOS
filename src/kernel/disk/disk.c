#include "disk.h"

#include <stddef.h>
#include <hardware/pci/pci.h>

#include "config.h"
#include "status.h"
#include "disk/driver/ataPio.h"
#include "memory/kheap/kheap.h"
#include "string/string.h"

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

static void insertAtaPioDisk(struct diskDriver* driver, uint16_t commandPort, uint16_t devConPort, uint16_t select)
{
	//We found a disk. YAYYYYYYYYYYY :3
	struct disk* disk = (struct disk*)kzalloc(sizeof(struct disk));

	disk->id = currentDisk;
	disk->type = DISK_TYPE_PHYSICAL;
	disk->driver = driver;
	if(ataPioAttach(disk, commandPort, select, devConPort) < 0)
	{
		kzfree(disk);
	}
	else
	{
		char* modelString = disk->driver->getModelString(disk->driver->private);
		strncpy(disk->name, modelString, sizeof(disk->name));
		kzfree(modelString);
		insertDisk(disk);
	}
}

//See if a disk is connected to legacy ports
static void scanLegacyPorts()
{
	//Check if we have a driver for ATAPIO legacy ports
	struct diskDriver* driver = getDriver(DISK_DRIVER_TYPE_ATA_LEGACY);
	if(driver != NULL)
	{
		//TODO: probe all legacy ports lol
		int diskAvailable = ataPioProbePort(0x1F0, 0x206);
		if(diskAvailable > 0)
		{
			insertAtaPioDisk(driver, 0x1F0, 0x206, 0xA);
		}
	}
}
//See if we have a native ide controller connected
static void scanPciBusAtaNative()
{
	//Check if we have a driver for ATAPIO native ports
	struct diskDriver* driver = getDriver(DISK_DRIVER_TYPE_ATA_NATIVE);
	if (driver == NULL){return;}

	struct pciDevice** ideControllers = getAllPciDevicesByClass(PCI_CLASS_MASS_STORAGE_CONTROLLER, PCI_SUBCLASS_MA_IDE_CONTROLLER, 0x8F);

	uint8_t count = 0;
	while (ideControllers[count] != NULL)
	{
		for (uint8_t i = 0; i < 3; i+=2)
		{
			struct pciBarInfo* barCommand = getPciBarInfo(ideControllers[count], i);
			struct pciBarInfo* barDevcon = getPciBarInfo(ideControllers[count], i+1);

			uint16_t select = i < 1 ? 0xA : 0xB;

			if (barCommand->isIo)
			{
				int diskAvailable = ataPioProbePort(barCommand->base, barDevcon->base);
				if (diskAvailable == 1)
				{
					insertAtaPioDisk(driver, barCommand->base, barDevcon->base, select);
				}
			}
		}
		count++;
	}
}

//This function trys to find all available disks on the system
static void scanDisks()
{
	scanLegacyPorts();
	scanPciBusAtaNative();
	//TODO: scan pci bus etc
}

int diskInit()
{
	scanDisks();
	return 0;
}

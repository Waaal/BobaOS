#include "disk.h"

#include <stddef.h>
#include <hardware/pci/pci.h>
#include <memory/memory.h>

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

static void insertAtaPioDisk(struct diskDriver* driver, uint16_t commandPort, uint16_t devConPort, uint16_t select)
{
	//We found a disk. YAYYYYYYYYYYY :3
	struct disk* disk = (struct disk*)kzalloc(sizeof(struct disk));

	disk->id = currentDisk;
	disk->type = DISK_TYPE_PHYSICAL;
	disk->driver = copyDriver(driver);
	if(driver == NULL || ataPioAttach(disk, commandPort, select, devConPort) < 0)
	{
		kzfree(disk);
	}
	else
	{
		struct diskInfo info = disk->driver->getInfo(disk->driver->private);

		strncpy(disk->name, info.name, sizeof(disk->name));
		disk->size = info.size;

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
		//Master
		int diskAvailable = ataPioProbePort(0x1F0, 0x206);
		if(diskAvailable > 0)
		{
			insertAtaPioDisk(driver, 0x1F0, 0x206, 0xA);
			print("  Found ATA_LEGACY (Master) at 0x1F0\n");
		}
		//Slave
		diskAvailable = ataPioProbePort(0x170, 0x206);
		if(diskAvailable > 0)
		{
			insertAtaPioDisk(driver, 0x170, 0x206, 0xB);
			print("  Found ATA_LEGACY (Slave) at 0x1F0\n");
		}
	}
}

/*
//See if we have a legacy ide controller connected
static void scanPciBusAtaLegacy()
{
	//Check if we have a driver for ATAPIO legacy ports
	struct diskDriver* driver = getDriver(DISK_DRIVER_TYPE_ATA_LEGACY);
	if (driver == NULL){return;}

	struct pciDevice** ideControllers = getAllPciDevicesByClass(PCI_CLASS_MASS_STORAGE_CONTROLLER, PCI_SUBCLASS_MA_IDE_CONTROLLER, 0x80);

	uint8_t count = 0;
	while (ideControllers[count] != NULL)
	{
		for (uint8_t i = 0; i < 3; i+=2)
		{
			struct pciBarInfo* barCommand = getPciBarInfo(ideControllers[count], i);
			struct pciBarInfo* barDevcon = getPciBarInfo(ideControllers[count], i+1);

			kprintf("%u, barCommand %x\n", i, barCommand->base);

			uint16_t select = i < 1 ? 0xA : 0xB;

			if (barCommand->isIo)
			{
				int diskAvailable = ataPioProbePort(barCommand->base, barDevcon->base);
				if (diskAvailable == 1)
				{
					insertAtaPioDisk(driver, barCommand->base, barDevcon->base, select);
					kprintf("  Found ATA_NATIVE at %x\n", barCommand->base);
				}
			}
		}
		count++;
	}
}
*/

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

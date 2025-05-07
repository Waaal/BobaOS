#include "ataPio.h"

#include <stddef.h>
#include <stdint.h>

#include "disk/diskDriver.h"
#include "status.h"
#include "memory/kheap/kheap.h"
#include "memory/memory.h"
#include "io/io.h"

struct ataPioPrivate
{
	uint16_t port;
	uint16_t select; 
};

static int ataPioRead(uint64_t lba, void* out);
struct diskDriver driver =
{
	.type = DISK_DRIVER_TYPE_ATA_LEGACY,
	.read = ataPioRead
};

static void wait(uint16_t base)
{
	//wait for some time by reading the status register 4 times
	inb(base + ATAPIO_REG_STATUS);
	inb(base + ATAPIO_REG_STATUS);
	inb(base + ATAPIO_REG_STATUS);
	inb(base + ATAPIO_REG_STATUS);
}

static void resetDrive(uint16_t base)
{	
	//set SRST
	outb(base + ATAPIO_REG_DEVICECON, ATAPIO_DEVICECON_SRST);
	wait(base);
	// clear SRST
	outb(base + ATAPIO_REG_DEVICECON, 0x00); 
} 

static void selectDrive(uint16_t base, uint16_t select)
{
	outb(base + ATAPIO_REG_DEVICECON, select);
}

static void disableInterruptSend(uint16_t base, uint16_t select)
{
	selectDrive(base, select);
	wait(base);
	outb(base + ATAPIO_REG_DEVICECON, 0x02);
}

static int ataPioRead(uint64_t lba, void* out)
{
	return 0;
}

struct diskDriver* registerAtaPioDriver()
{
	//copy the driver since we can have multiple disk who use the ataPio driver with different settings
	struct diskDriver* cpyDriver = (struct diskDriver*)kzalloc(sizeof(struct diskDriver));
	memcpy(cpyDriver,&driver, sizeof(struct diskDriver));	

	return cpyDriver;
}

int ataPioProbeLegacyPorts(uint16_t base)
{
	resetDrive(base);
	
	uint8_t status = inb(base + ATAPIO_REG_STATUS);
	if(status == 0xFF || status == 0x00)
	{
		return 0;
	}
	return 1;
}

//Attaches a ataPIO driver to the disk and disable IRQs for PIO mode
int ataPioAttach(struct disk* disk, uint16_t base, uint16_t select)
{
	struct ataPioPrivate* private = (struct ataPioPrivate*)kzalloc(sizeof(struct ataPioPrivate));
	if(private != NULL)
	{
		private->port = base;
		private->select = select;
		disk->driver->private = private;
		
		disableInterruptSend(base, select);
		return 0;
	}
	return -ENMEM;
}

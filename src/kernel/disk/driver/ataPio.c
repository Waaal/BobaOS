#include "ataPio.h"

#include <print.h>
#include <stddef.h>
#include <stdint.h>
#include <string/string.h>

#include "disk/diskDriver.h"
#include "status.h"
#include "memory/kheap/kheap.h"
#include "memory/memory.h"
#include "io/io.h"

struct ataPioPrivate
{
	uint16_t port;
	uint16_t select;
	uint16_t deviceCon;
	uint8_t lba48Enabled;
};

static int ataPioRead(uint64_t lba, uint64_t total, void* out, void* private);
static int ataPioWrite(uint64_t lba, uint64_t total, void* in, void* private);
static struct diskInfo ataPioIdentify(void* private);

struct diskDriver driver =
{
	.type = DISK_DRIVER_TYPE_ATA_LEGACY,
	.getInfo = ataPioIdentify,
	.read = ataPioRead,
	.write = ataPioWrite
};

static void wait(uint16_t base)
{
	//wait for some time by reading the status register 4 times
	inb(base + ATAPIO_REG_STATUS);
	inb(base + ATAPIO_REG_STATUS);
	inb(base + ATAPIO_REG_STATUS);
	inb(base + ATAPIO_REG_STATUS);
}

static void resetDrive(uint16_t base, uint16_t deviceCon)
{	
	//set SRST
	outb(base + deviceCon, ATAPIO_DEVICECON_SRST);
	wait(base);
	// clear SRST
	outb(base + deviceCon, 0x00);
} 

static void selectDrive(uint16_t base, uint16_t select, uint16_t deviceCon)
{
	outb(base + deviceCon, select);
}

static void disableInterruptSend(uint16_t base, uint16_t select, uint16_t deviceCon)
{
	selectDrive(base, select, deviceCon);
	wait(base);
	outb(base + deviceCon, 0x02);
}

static void ataPioWaitBsy(uint16_t base)
{
	while ((inb(base + ATAPIO_REG_STATUS) & ATAPIO_STATUS_BSY));
}

static void ataPioWaitDrq(uint16_t base)
{
	while(!(inb(base + ATAPIO_REG_STATUS) & ATAPIO_STATUS_DRQ));
}

static uint8_t checkStatusError(uint16_t base)
{
	if (inb(base + ATAPIO_REG_STATUS) & ATAPIO_STATUS_ERR)
		return 1;
	return 0;
}

static uint8_t getErrorCode(uint16_t base)
{
	return inb(base + ATAPIO_REG_ERROR);
}

static struct diskInfo ataPioIdentify(void* private)
{
	struct ataPioPrivate* pr = (struct ataPioPrivate*)private;

	selectDrive(pr->port, pr->select, pr->deviceCon);
	outb(pr->port + ATAPIO_REG_SECTOR, 0);
	outb(pr->port + ATAPIO_REG_LBA0, 0);
	outb(pr->port + ATAPIO_REG_LBA1, 0);
	outb(pr->port + ATAPIO_REG_LBA2, 0);
	outb(pr->port + ATAPIO_REG_COMMAND, ATAPIO_COMMAND_IDENTIFY);
	ataPioWaitBsy(pr->port);
	ataPioWaitDrq(pr->port);

	uint16_t dataBuffer[256];
	for (uint32_t i = 0; i < 256; i++)
		dataBuffer[i] = inw(pr->port);

	//Check lba48
	uint8_t lba48Enabled = (uint8_t)((dataBuffer[83] & 0x400) >> 10);
	pr->lba48Enabled = lba48Enabled;

	//Extract mdoel string
	char* model = kzalloc(42);
	for (uint8_t i = 0; i < 20; i++)
	{
		model[i*2] = (char)(dataBuffer[27+i] >> 8);
		model[i*2+1] = (char)(dataBuffer[27+i] & 0xFF);
	}
	model[41] = 0;
	//Extract total LBAs
	uint32_t totalLBA = 0;
	totalLBA = dataBuffer[60];
	totalLBA |= dataBuffer[61] << 16;

	//Fill diskinfo return struct
	struct diskInfo info;
	strncpy(info.name, model, 42);
	info.size = totalLBA * 512;

	return info;
}

static void cacheFlush(uint16_t port)
{
	outb(port+ ATAPIO_REG_COMMAND, 0xE7);
	ataPioWaitBsy(port);
}

static int ataPioWrite(uint64_t lba, uint64_t total, void* in, void* private)
{
	struct ataPioPrivate* pr = (struct ataPioPrivate*)private;

	selectDrive(pr->port, pr->select, pr->deviceCon);
	ataPioWaitBsy(pr->port);

	outb(pr->port + ATAPIO_REG_ERROR, 0x00);
	outb(pr->port + ATAPIO_REG_SECTOR, total);
	outb(pr->port + ATAPIO_REG_LBA0, (unsigned char)(lba & 0xFF)); //Bits 0 - 7 from address
	outb(pr->port + ATAPIO_REG_LBA1, (unsigned char)(lba >> 8) & 0xFF); //Bits 8 - 15 from address
	outb(pr->port + ATAPIO_REG_LBA2, (unsigned char)(lba >> 16) & 0xFF); //Bits 16- 23 from address
	outb(pr->port + ATAPIO_REG_DRIVESELECT, 0xE0 | ((lba >> 24) & 0x0F)); //Bits 24- 27 from address, bits 28 - 31 are set to 1110
	outb(pr->port + ATAPIO_REG_COMMAND, ATAPIO_COMMAND_WRITE_SECTORS);

	uint16_t* ptr = (uint16_t*)in;
	for (uint64_t b = 0; b < total; b++)
	{
		ataPioWaitDrq(pr->port);

		if (checkStatusError(pr->port))
		{
			kprintf("[ERROR-ATAPIO]: Ata pio returned error code %u\n", getErrorCode(pr->port));
			return -EHARDWARE;
		}

		for (uint16_t i = 0; i < 256; i++)
		{
			outw(pr->port, *ptr);
			ptr++;
		}
	}
	cacheFlush(pr->port);
	return 0;
}

static int ataPioRead(uint64_t lba, uint64_t total, void* out, void* private)
{
	struct ataPioPrivate* pr = (struct ataPioPrivate*)private;

	selectDrive(pr->port, pr->select, pr->deviceCon);
	ataPioWaitBsy(pr->port);

	outb(pr->port + ATAPIO_REG_ERROR, 0x00);
	outb(pr->port + ATAPIO_REG_SECTOR, total);
	outb(pr->port + ATAPIO_REG_LBA0, (unsigned char)(lba & 0xFF)); //Bits 0 - 7 from address
	outb(pr->port + ATAPIO_REG_LBA1, (unsigned char)(lba >> 8) & 0xFF); //Bits 8 - 15 from address
	outb(pr->port + ATAPIO_REG_LBA2, (unsigned char)(lba >> 16) & 0xFF); //Bits 16- 23 from address
	outb(pr->port + ATAPIO_REG_DRIVESELECT, 0xE0 | ((lba >> 24) & 0x0F)); //Bits 24- 27 from address, bits 28 - 31 are set to 1110
	outb(pr->port + ATAPIO_REG_COMMAND, ATAPIO_COMMAND_READ_SECTORS);

	uint16_t* ptr = (uint16_t*)out;
	for (uint64_t b = 0; b < total; b++)
	{
		ataPioWaitDrq(pr->port);

		if (checkStatusError(pr->port))
		{
			kprintf("[ERROR-ATAPIO]: Ata pio returned error code %u\n", getErrorCode(pr->port));
			return -EHARDWARE;
		}

		for (uint16_t i = 0; i < 256; i++)
		{
			*ptr = inw(pr->port);
			ptr++;
		}
	}

	return 0;
}

int ataPioProbePort(uint16_t base, uint16_t deviceCon)
{
	resetDrive(base, deviceCon);
	
	uint8_t status = inb(base + ATAPIO_REG_STATUS);
	if(status == 0xFF || status == 0x00)
	{
		return 0;
	}
	return 1;
}

struct diskDriver* registerAtaPioDriver()
{
	return &driver;
}

//Attaches a ataPIO driver to the disk and disable IRQs for PIO mode
int ataPioAttach(struct disk* disk, uint16_t base, uint16_t select, uint16_t deviceCon)
{
	struct ataPioPrivate* private = (struct ataPioPrivate*)kzalloc(sizeof(struct ataPioPrivate));
	if(private != NULL)
	{
		private->port = base;
		private->select = select;
		private->deviceCon = deviceCon;
		disk->driver->private = private;
		
		disableInterruptSend(base, select, deviceCon);
		return 0;
	}
	return -ENMEM;
}

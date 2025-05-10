#ifndef DISK_DRIVER_H
#define DISK_DRIVER_H

#include <stdint.h>

enum diskDriverType
{
	DISK_DRIVER_TYPE_ATA_LEGACY,
	DISK_DRIVER_TYPE_ATA_NATIVE,
	DISK_DRIVER_TYPE_SATA_AHCI
};

typedef int (*DISK_READ)(uint64_t lba, void* out);
typedef char* (*DISK_GETMODELSTRING)(void* private);

struct diskDriver
{
	enum diskDriverType type;
	DISK_GETMODELSTRING getModelString;
	DISK_READ read;

	void* private;
};

int diskDriverInit();
struct diskDriver* getDriver(enum diskDriverType driverType);
#endif

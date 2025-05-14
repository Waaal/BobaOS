#ifndef DISK_DRIVER_H
#define DISK_DRIVER_H

#include <stdint.h>

enum diskDriverType
{
	DISK_DRIVER_TYPE_ATA_LEGACY,
	DISK_DRIVER_TYPE_ATA_NATIVE,
	DISK_DRIVER_TYPE_SATA_AHCI
};

struct diskInfo
{
	char name[64];
	uint64_t size;
};

typedef int (*DISK_READ)(uint64_t lba, uint64_t total, void* out, void* private);
typedef int (*DISK_WRITE)(uint64_t lba, uint64_t total, void* in, void* private);
typedef struct diskInfo (*DISK_GETINFO)(void* private);

struct diskDriver
{
	enum diskDriverType type;
	DISK_GETINFO getInfo;
	DISK_READ read;
	DISK_WRITE write;

	void* private;
};

int diskDriverInit();
struct diskDriver* getDriver(enum diskDriverType driverType);
#endif

#ifndef DISK_DRIVER_H
#define DISK_DRIVER_H

#include <stdint.h>


enum diskDriverType
{
	DISK_DRIVER_TYPE_ATA_LEGACY,
	DISK_DRIVER_TYPE_AHCI
};

struct diskInfo
{
	char name[64];
	uint64_t size;
};

struct disk;
typedef int(*SCAN_FOR_DISK)(struct disk** diskList, int* diskFoundCount, uint16_t nextId);
typedef struct diskInfo (*DISK_GETINFO)(void* private);
typedef int (*DISK_READ)(uint64_t lba, uint64_t total, void* out, void* private);
typedef int (*DISK_WRITE)(uint64_t lba, uint64_t total, void* in, void* private);

struct diskDriver
{
	enum diskDriverType type;
	DISK_GETINFO getInfo;
	SCAN_FOR_DISK scanForDisk;
	DISK_READ read;
	DISK_WRITE write;

	void* private;
};

int diskDriverInit();
struct diskDriver* copyDriver(struct diskDriver* drv);
struct diskDriver* getDriver(enum diskDriverType driverType);
#endif

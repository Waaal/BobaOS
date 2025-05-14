#ifndef DISK_H
#define DISK_H

#include <stdint.h>

#include "diskDriver.h"

enum diskType
{
	DISK_TYPE_PHYSICAL = 0x1
};

struct disk
{
	uint8_t id;
	char name[64];
	uint64_t size;
	enum diskType type;
	struct diskDriver* driver;
};

int diskInit();
struct disk* diskGet(uint8_t id);

#endif

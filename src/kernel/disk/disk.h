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
	char name[32];
	enum diskType type;
	struct diskDriver driver;
};

#endif

#include "diskDriver.h"

#include <macros.h>
#include <stddef.h>
#include <memory/memory.h>
#include <memory/kheap/kheap.h>

#include "config.h"
#include "status.h"
#include "driver/ataPio.h"
#include "driver/ahci.h"

struct diskDriver* driverList[BOBAOS_MAX_DISK_DRIVER];
uint16_t nextDriver = 0;

static int insertDriver(struct diskDriver* driver)
{
	if(nextDriver == BOBAOS_MAX_DISK_DRIVER)
	{
		return -ENMEM;
	}
	driverList[nextDriver] = driver;
	nextDriver++;

	return 0;
}

//load all the driver that got statically compiled with BobaOS
static int loadStaticDriver()
{
	int ret = 0;

	ret = insertDriver(registerAtaPioDriver());
	if (ret == 0)
		ret = insertDriver(registerAHCI());
	return ret;
}

struct diskDriver* getDriver(enum diskDriverType driverType)
{
	struct diskDriver* ret =NULL;
	for(uint16_t i = 0; i < nextDriver; i++)
	{	
		if(driverList[i]->type == driverType)
		{
			return driverList[i];
		}
	}
	return ret;
}

struct diskDriver* copyDriver(struct diskDriver* drv)
{
	struct diskDriver* ret = kzalloc(sizeof(struct diskDriver));
	RETNULL(ret);

	memcpy(ret, drv, sizeof(struct diskDriver));
	return ret;
}

int diskDriverInit()
{
	int ret = 0;

	ret = loadStaticDriver();
	if(ret > 0){return ret;}
	
	return ret;
}

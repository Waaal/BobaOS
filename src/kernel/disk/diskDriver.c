#include "diskDriver.h"

#include <stddef.h>

#include "config.h"
#include "driver/ataPio.h"
#include "status.h"

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

	ret = insertDriver(registerAtaPioDriver(DISK_DRIVER_TYPE_ATA_LEGACY));
	if(ret < 0){return ret;}

	ret = insertDriver(registerAtaPioDriver((DISK_DRIVER_TYPE_ATA_NATIVE)));
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

int diskDriverInit()
{
	int ret = 0;

	ret = loadStaticDriver();
	if(ret > 0){return ret;}
	
	return ret;
}

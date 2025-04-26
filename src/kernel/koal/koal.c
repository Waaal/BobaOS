#include "koal.h"

#include <stdint.h>
#include <stddef.h>

#include "config.h"
#include "memory/memory.h"
#include "status.h"
#include "string/string.h"

struct kernelOutputAbstractionLayer* kernelOutputAbstractionLayer[BOBAOS_MAX_KOAL_ENTRIES]; 
uint8_t currentIndex = 0;
int selectedOutput = -1;

static int checkCurrentSelected()
{
	if(selectedOutput < 0 || kernelOutputAbstractionLayer[selectedOutput] == NULL)
	{
		return -ENFOUND;
	}
	return 0;
}

int koalSelectCurrentOutputByName(const char* name)
{
	for(uint8_t i = 0; i < currentIndex; i++)
	{
		if(strcmp(name, kernelOutputAbstractionLayer[i]->name) == 0)
		{
			selectedOutput = i;
			return 0;
		}
	}
	return -ENFOUND;
}

int koalAttach(struct kernelOutputAbstractionLayer* koal)
{
	if(currentIndex >= BOBAOS_MAX_KOAL_ENTRIES)
	{
		return -ENMEM;
	}

	kernelOutputAbstractionLayer[currentIndex] = koal;
	currentIndex++;
	return 0;
}

int koalPrintChar(const char c)
{
	int ret = checkCurrentSelected();
	if(ret < 0)
	{
		return ret;
	}
	return kernelOutputAbstractionLayer[selectedOutput]->outChar(c);
}

int koalPrint(const char* str)
{
	int ret = 0;
	for(uint64_t i = 0; i < strlen(str); i++)
	{
		ret = koalPrintChar(str[i]);
		if(ret < 0)
		{
			break;
		}
	}	
	return ret;
}

void koalInit()
{
	memset(kernelOutputAbstractionLayer, 0x0, 0x8 * BOBAOS_MAX_KOAL_ENTRIES);
}

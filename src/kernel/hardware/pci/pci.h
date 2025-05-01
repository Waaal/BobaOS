#ifndef PCI_H
#define PCI_H

#include <stdint.h>

#define PCI_CONFIG_ADDRESS 0xCF8
#define PCI_CONFIG_DATA 0xCFC

struct pciDevice
{
	uint16_t deviceId;
	uint16_t vendorId;
	uint16_t status;
	uint16_t command;
	uint8_t classCode;
	uint8_t subClass;
	uint8_t prog;
	uint8_t revisionId;
	uint8_t bist;
	uint8_t headerType;
	uint8_t latencyTimer;
	uint8_t cacheLineSize;
};

void pciInit();

#endif

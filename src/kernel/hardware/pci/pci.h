#ifndef PCI_H
#define PCI_H

#include <stdint.h>

#define PCI_CONFIG_ADDRESS 0xCF8
#define PCI_CONFIG_DATA 0xCFC

#define PCI_HEADER_TYPE_DEVICE  0x0
#define PCI_HEADER_TYPE_PCI_BRIDGE 0x1
#define PCI_HEADER_TYPE_CARD_BUS_BRIDGE 0x2
#define PCI_HEADER_TYPE_MULTI_FUNCTION 0x80

struct pciDevice
{
	uint8_t bus;
	uint8_t device;
	uint8_t function;
	struct pciHeader* header;
};

struct pciHeader
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
	union
	{
		struct pciHeaderDevice* deviceHeader;
		struct pciHeaderPciBridge* bridgeHeader;
		struct pciHeaderCardBridge* cardHeader;
	} extendedHeader;
};

struct pciHeaderDevice
{
	uint32_t BAR0;
	uint32_t BAR1;
	uint32_t BAR2;
	uint32_t BAR3;
	uint32_t BAR4;
	uint32_t BAR5;
	uint32_t CisPointer;
	uint16_t subSystemId;
	uint16_t subSystemVendorId;
	uint32_t expansionRomAddress;
	uint8_t capabilitiesPointer;
	uint8_t maxLatency;
	uint8_t minGrant;
	uint8_t interruptPin;
	uint8_t interruptLine;
};

struct pciHeaderPciBridge
{
};

struct pciHeaderCardBridge
{
};

void pciInit();

#endif

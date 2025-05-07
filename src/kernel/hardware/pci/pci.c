#include "pci.h"

#include <stddef.h>

#include "config.h"
#include "io/io.h"
#include "memory/kheap/kheap.h"
#include "print.h"
#include "status.h"
#include "stdint.h"

struct pciDevice* pciDevices[BOBOAOS_MAX_PCI_DEVICES];
uint16_t nextDevicesIndex = 0;

static int pushDevice(struct pciDevice* device)
{
	if(nextDevicesIndex == BOBOAOS_MAX_PCI_DEVICES)
	{
		return -ENMEM;
	}

	pciDevices[nextDevicesIndex] = device;
	nextDevicesIndex++;
	
	return 0;
}

static uint32_t readConfig(uint8_t bus, uint8_t device, uint8_t function, uint8_t offset)
{
	uint32_t out = (1 << 31) | (bus << 16) | (device << 11) | (function << 8) | (offset & 0xFC);
	
	outd(PCI_CONFIG_ADDRESS, out);
	return ind(PCI_CONFIG_DATA);
}

static void writeConfig(uint8_t bus, uint8_t device, uint8_t function, uint8_t offset, uint32_t value)
{
	uint32_t out = (1 << 31) | (bus << 16) | (device << 11) | (function << 8) | (offset & 0xFC);

	outd(PCI_CONFIG_ADDRESS, out);
	outd(PCI_CONFIG_DATA, value);
}

static struct pciHeaderDevice* readDeviceHeader(uint8_t bus, uint8_t device, uint8_t function)
{
	struct pciHeaderDevice* header = (struct pciHeaderDevice*)kzalloc(sizeof(struct pciHeaderDevice*));
	
	uint32_t bar0 = readConfig(bus, device, function, 0x10);
	uint32_t bar1 = readConfig(bus, device, function, 0x14);
	uint32_t bar2 = readConfig(bus, device, function, 0x18);
	uint32_t bar3 = readConfig(bus, device, function, 0x1C);
	uint32_t bar4 = readConfig(bus, device, function, 0x20);
	uint32_t bar5 = readConfig(bus, device, function, 0x24);

	uint32_t cisPointer = readConfig(bus, device, function, 0x28);
	uint32_t subsystemId_subsystemVendorId = readConfig(bus, device, function, 0x2C);
	uint32_t romAddress = readConfig(bus, device, function, 0x30);
	uint32_t reserved_pointer = readConfig(bus, device, function, 0x34);
	uint32_t latency_grant_pin_line = readConfig(bus, device, function, 0x3c);
	
	header->BAR0 = bar0;
	header->BAR1 = bar1;
	header->BAR2 = bar2;
	header->BAR3 = bar3;
	header->BAR4 = bar4;
	header->BAR5 = bar5;

	header->CisPointer = cisPointer;
	header->subSystemId = subsystemId_subsystemVendorId >> 16;
	header->subSystemVendorId = subsystemId_subsystemVendorId;
	header-> expansionRomAddress = romAddress;
	header->capabilitiesPointer = reserved_pointer;
	header->maxLatency = latency_grant_pin_line >> 24;
	header->minGrant = latency_grant_pin_line >> 16;
	header->interruptPin = latency_grant_pin_line >> 8;
	header->interruptLine = latency_grant_pin_line;

	return header;
}

static struct pciDevice* readPciDevice(uint8_t bus, uint8_t device, uint8_t function)
{
	struct pciDevice* pciDevice = (struct pciDevice*)kzalloc(sizeof(struct pciDevice));
	struct pciHeader* header = (struct pciHeader*)kzalloc(sizeof(struct pciHeader));

	uint32_t device_vendor = readConfig(bus, device, function, 0x0);
	uint32_t status_command = readConfig(bus, device, function, 0x4);
	uint32_t class_subClass_prog_revision = readConfig(bus, device, function, 0x8);
	uint32_t bist_header_latency_cache = readConfig(bus, device, function, 0xC);

	header->deviceId = device_vendor >> 16;
	header->vendorId = device_vendor;

	header->status = status_command >> 16;
	header->command = status_command;

	header->classCode = class_subClass_prog_revision >> 24;
	header->subClass = class_subClass_prog_revision >> 16;
	header->prog = class_subClass_prog_revision >> 8;
	header->revisionId = class_subClass_prog_revision;
	
	header->bist = bist_header_latency_cache >> 24;
	header->headerType = bist_header_latency_cache >> 16;
	header->latencyTimer = bist_header_latency_cache >> 8;
	header->cacheLineSize = bist_header_latency_cache;
	
	switch(header->headerType)
	{
		case PCI_HEADER_TYPE_DEVICE:
			{
				struct pciHeaderDevice* deviceHeader = readDeviceHeader(bus, device, function);
				header->extendedHeader.deviceHeader = deviceHeader;
				break;
			}
		case PCI_HEADER_TYPE_PCI_BRIDGE:
			{
				kprintf("  [PCI-Error: ] No support for pci to pci bridges. Bus %u device %u\n", bus, device);
				goto error;
			}
		case PCI_HEADER_TYPE_CARD_BUS_BRIDGE:
			{
				kprintf("  [PCI-Error ]: No support for pci to card bus bridges. Bus %u device %u\n", bus, device);
				goto error;
			}
			case PCI_HEADER_TYPE_MULTI_FUNCTION:
			{
				for(uint8_t i = 1; i < 8; i++)
				{
					uint16_t vendor = (uint16_t)(readConfig(bus, device, i, 0x0) & 0xFFFF);
					if(vendor == 0xFFFF){continue;}
					struct pciDevice* subDevice = readPciDevice(bus, device, i);
					if(subDevice != NULL)
					{
						pushDevice(subDevice);
						kprintf("  [PCI-SubDevice]: %u Class: %u SubClass: %u\n", subDevice->header->deviceId, subDevice->header->classCode, subDevice->header->subClass);
					}
				}
				break;
			}
			default:
			{
				kprintf("  [PCI-Error ]: PCI header unknown. Bus %u device %u\n", bus, device);
				goto error;
			}
	}
	
	pciDevice->header = header;
	pciDevice->bus = bus;
	pciDevice->device = device;
	pciDevice->function = function;

	return pciDevice;

error:
	kzfree(pciDevice);
	kzfree(header);
	return (void*)0x0;
}

static void pciScanBus()
{
	for(uint8_t bus = 0; bus < 255; bus++)
	{
		for(uint8_t device = 0; device < 32; device++)
		{
			uint16_t vendor = ((uint16_t)(readConfig(bus, device, 0, 0x0) & 0xFFFF));
			if(vendor != 0xFFFF)
			{
				struct pciDevice* pciDevice = readPciDevice(bus, device, 0);
				if(pciDevice != NULL)
				{
					pushDevice(pciDevice);
					kprintf("  [PCI-Device]: %u Class: %u SubClass: %u ProgIf: %u\n", pciDevice->header->deviceId, pciDevice->header->classCode, pciDevice->header->subClass, pciDevice->header->prog);
				}
			}
		}
	}
}

struct pciDevice* getPciDeviceByClass(enum pciClasses class, enum pciSubClasses subClass, uint8_t progIf)
{
	struct pciDevice* ret = (void*)0x0;
	for(uint16_t i = 0; i < nextDevicesIndex; i++)
	{
		if(pciDevices[i]->header->classCode == class && pciDevices[i]->header->subClass == (subClass & 0xFF) && pciDevices[i]->header->prog == progIf)
		{
			ret = pciDevices[i];
			break;
		}
	}

	return ret;
}

struct pciDevice* getPciDeviceByVendor(uint16_t vendorId, uint16_t deviceId)
{
	struct pciDevice* ret = (void*)0x0;
	for(uint16_t i = 0; i < nextDevicesIndex; i++)
	{
		if(pciDevices[i]->header->vendorId == vendorId && pciDevices[i]->header->deviceId == deviceId)
		{
			ret = pciDevices[i];
			break;
		}
	}

	return ret;
}

struct pciDevice** getAllPciDevicesByClass(enum pciClasses class, enum pciSubClasses subClass, uint8_t progIf)
{
	struct pciDevice** arr = (struct pciDevice**)kzalloc(BOBOAOS_MAX_PCI_DEVICES * sizeof(struct pciDevice));
	
	uint8_t counter = 0;
	for(uint8_t i = 0; i< nextDevicesIndex; i++)
	{
		if(pciDevices[i]->header->classCode == class && pciDevices[i]->header->subClass == (subClass & 0xFF) && pciDevices[i]->header->prog == progIf)
		{
			arr[counter] = pciDevices[i];
			counter++;
		}
	}
	return arr;
}

struct pciBarInfo* getPciBarInfo(struct pciDevice* pciDevice, uint8_t bar)
{
	uint8_t offset = 0x10 + (bar*4);

	uint32_t originalBAR = readConfig(pciDevice->bus, pciDevice->device, pciDevice->function, offset);
	writeConfig(pciDevice->bus, pciDevice->device, pciDevice->function, offset, 0xFFFFFFFF);

	uint32_t mask = readConfig(pciDevice->bus, pciDevice->device, pciDevice->function, offset);
	writeConfig(pciDevice->bus, pciDevice->device, pciDevice->function, offset, originalBAR);
	
	uint8_t isIo = (originalBAR & 1) ? 1 : 0;
	struct pciBarInfo* info = (struct pciBarInfo*)kzalloc(sizeof(struct pciBarInfo));
	uint32_t sizeMask = isIo ? 0xFFFFFFFC : 0xFFFFFFF0;

	info->isIo = isIo;
	info->size = ~(mask & sizeMask) + 1; 
	info->base = originalBAR & sizeMask;
	return info;
}

void updateStatusRegister(struct pciDevice* pciDevice)
{
	uint32_t status_command = readConfig(pciDevice->bus, pciDevice->device, pciDevice->function, 0x4);
	pciDevice->header->status = status_command >> 16;
}

void pciInit()
{
	print("PCI Scan:\n");
	pciScanBus();
};

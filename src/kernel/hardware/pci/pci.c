#include "pci.h"

#include "io/io.h"
#include "memory/kheap/kheap.h"
#include "print.h"

static uint32_t readConfig(uint8_t bus, uint8_t device, uint8_t function, uint8_t offset)
{
	uint32_t out = (1 << 31) | (bus << 16) | (device << 11) | (function << 8) | (offset & 0xFC);
	
	outd(PCI_CONFIG_ADDRESS, out);
	return ind(PCI_CONFIG_DATA);
}

static struct pciDevice* readPciDevice(uint8_t bus, uint8_t device, uint8_t function)
{
	struct pciDevice* pci = (struct pciDevice*)kzalloc(sizeof(struct pciDevice));

	uint32_t device_vendor = readConfig(bus, device, function, 0x0);
	uint32_t status_command = readConfig(bus, device, function, 0x4);
	uint32_t class_subClass_prog_revision = readConfig(bus, device, function, 0x8);
	uint32_t bist_header_latency_cache = readConfig(bus, device, function, 0xC);

	pci->deviceId = device_vendor >> 16;
	pci->vendorId = device_vendor;

	pci->status = status_command >> 16;
	pci->command = status_command;

	pci->classCode = class_subClass_prog_revision >> 24;
	pci->subClass = class_subClass_prog_revision >> 16;
	pci->prog = class_subClass_prog_revision >> 8;
	pci->revisionId = class_subClass_prog_revision;
	
	pci->bist = bist_header_latency_cache >> 24;
	pci->headerType = bist_header_latency_cache >> 16;
	pci->latencyTimer = bist_header_latency_cache >> 8;
	pci->cacheLineSize = bist_header_latency_cache;

	return pci;
}

void pciInit()
{
	for(uint8_t bus = 0; bus < 255; bus++)
	{
		for(uint8_t device = 0; device < 32; device++)
		{
			uint16_t vendor = ((uint16_t)(readConfig(bus, device, 0, 0x0) & 0xFFFF));
			if(vendor != 0xFFFF)
			{
				struct pciDevice* pciDevice = readPciDevice(bus, device, 0);
				kprintf("PCI DEVICE: %u Class: %u SubClass: %u\n", pciDevice->deviceId, pciDevice->classCode, pciDevice->subClass);
			}
		}
	}

	struct pciDevice* testDevice = readPciDevice(0,1,0);

	if(testDevice){}
};

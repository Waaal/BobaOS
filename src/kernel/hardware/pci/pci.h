#ifndef PCI_H
#define PCI_H

#include <stdint.h>

#define PCI_CONFIG_ADDRESS 0xCF8
#define PCI_CONFIG_DATA 0xCFC

#define PCI_HEADER_TYPE_DEVICE  0x0
#define PCI_HEADER_TYPE_PCI_BRIDGE 0x1
#define PCI_HEADER_TYPE_CARD_BUS_BRIDGE 0x2
#define PCI_HEADER_TYPE_MULTI_FUNCTION 0x80

enum pciClasses
{
	PCI_CLASS_UNCLASSIFIED = 0x0,
	PCI_CLASS_MASS_STORAGE_CONTROLLER = 0x1,
	PCI_CLASS_NETWORK_CONTROLLER = 0x2,
	PCI_CLASS_DISPLAY_CONTROLLER = 0x3,
	PCI_CLASS_MULTIMEDIA_CONTROLLER = 0x4,
	PCI_CLASS_MEMORY_CONTROLLER = 0x5,
	PCI_CLASS_BRIDGE = 0x6,
	PCI_CLASS_SIMPLE_COMMUNICATION_CONTROLLER = 0x7,
	PCI_CLASS_BASE_SYSTEM_PERIPHERAL = 0x8,
	PCI_CLASS_INPUT_DEVICE_CONTROLLER = 0x9,
	PCI_CLASS_DOCKING_STATION = 0xA,
	PCI_CLASS_PROCESSOR = 0xB,
	PCI_CLASS_SERIAL_BUS_CONTROLLER = 0xC,
	PCI_CLASS_WIRELESS_CONTROLLER = 0xD,
	PCI_CLASS_INTELLIGENT_CONTROLLER = 0xE,
	PCI_CLASS_SATELLITE_CONTROLLER = 0xF,
	PCI_CLASS_ENCRYPTION_CONTROLLER = 0x10,
	PCI_CLASS_SIGNAL_PROCESSING_CONTROLLER = 0x11
};

//The first 2 bytes are the actuall subclass and the last 2 bytes are the class
//Example: 0x0101 = 
// MASS STORAGE 		IDE CONTROLLER
// 0x01            		01
enum pciSubClasses
{
	PCI_SUBCLASS_UN_NON_VGA_COMPATIBLE = 				0x0000,
	PCI_SUBCLASS_UN_VGA_COMPATIBLE = 					0x0001,

	PCI_SUBCLASS_MA_IDE_CONTROLLER = 					0x0101,
	PCI_SUBCLASS_MA_FLOPPY_CONTROLLER = 				0x0102,
	PCI_SUBCLASS_MA_IPI_BUS_CONTROLLER =				0x0103,
	PCI_SUBCLASS_MA_RAID_CONTROLLER =					0x0104,
	PCI_SUBCLASS_MA_ATA_CONTROLLER =					0x0105,
	PCI_SUBCLASS_MA_SATA_CONTROLLER =					0x0106,
	PCI_SUBCLASS_MA_S_ATTACHED_SCSI_CONTROLLER = 		0x0107,
	PCI_SUBCLASS_MA_NON_VOLATILE_MEMORY_CONTROLLER = 	0x0108,
	PCI_SUBCLASS_MA_OTHER = 							0x0180
};

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

struct pciBarInfo
{
	uint8_t isIo;
	uint32_t base;
	uint32_t size;
};

void pciInit();
struct pciDevice* getPciDeviceByClass(enum pciClasses class, enum pciSubClasses subClass, uint8_t progIf);
struct pciDevice** getAllPciDevicesByClass(enum pciClasses class, enum pciSubClasses subClass, uint8_t progIf);
struct pciDevice* getPciDeviceByVendor(uint16_t vendorId, uint16_t deviceId);
struct pciBarInfo* getPciBarInfo(struct pciDevice* device, uint8_t bar);
void updateStatusRegister(struct pciDevice* pciDevice);

#endif

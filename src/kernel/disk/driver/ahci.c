#include "ahci.h"

#include <macros.h>
#include <stddef.h>
#include <memory/memory.h>
#include <memory/kheap/kheap.h>

#include "kernel.h"
#include "config.h"
#include "status.h"
#include "hardware/pci/pci.h"
#include "print.h"
#include "memory/paging/paging.h"

struct ahciPrivate
{
    HBA_PORT port;
};

static struct diskInfo ahciIdentifyCommand(void* private);
static int scanForDisk(struct disk** diskList, int* diskFoundCount, uint16_t nextId);

static struct diskDriver driver = {
    .type = DISK_DRIVER_TYPE_AHCI,
    .scanForDisk = scanForDisk,
    .getInfo = ahciIdentifyCommand,
    .read = NULL,
    .write = NULL
};

static int getFreeSlot(HBA_PORT port)
{
    uint8_t slots = port->sataActive | port->commandIssue;
    for (uint8_t i = 0; i < 32; i++)
    {
        if ((slots & 1) == 0)
            return i;
        slots >>=1;
    }
    return -ENFOUND
}

static void stopCommandSending(HBA_PORT port)
{
    port->commandAndStatus &= ~0x0001;
    port->commandAndStatus &= ~0x0010;

    while (1)
    {
        if ((port->commandAndStatus & 0x4000)) continue;
        if ((port->commandAndStatus & 0x8000)) continue;
        break;
    }
}

static void startCommandSending(HBA_PORT port)
{
    while (port->commandAndStatus & 0x8000);

    port->commandAndStatus |= 0x0010;
    port->commandAndStatus |= 0x0001;
}

static int remapPort(HBA_PORT port)
{
    stopCommandSending(port);

    void* addr = kzalloc(9472); //[1024 = (32* commandHeader)] + [256 = (1 * fis)] + [8192 = (32 * CommandTable with 8 PhysicalRegionDescriptorTables)]
    RETNULLERROR(addr, -ENMEM);

    uint64_t baseAddr = (uint64_t)virtualToPhysical(addr, getKernelPageTable());

    kprintf("    Map port commandHeader to: %x \n", baseAddr);

    port->commandListBase = baseAddr & 0xFFFFFFFF;
    port->commandListBaseUpper = (baseAddr >> 32) & 0xFFFFFFFF;
    baseAddr += 1024;

    kprintf("    Map port FIS to: %x \n", baseAddr);
    port->fisBase = baseAddr & 0xFFFFFFFF;
    port->fisBaseUpper = (baseAddr >> 32) & 0xFFFFFFFF;
    baseAddr += 256;

    kprintf("    Map commandTables: %x -> %x\n", baseAddr, baseAddr + (31*256));
    volatile struct commandHeader* cmdHeader = (volatile struct commandHeader*)((uint64_t)(((uint64_t)port->commandListBaseUpper << 32) | port->commandListBase));
    for (uint8_t i = 0; i < 32; i++)
    {
        cmdHeader[i].physicalRegionDescriptorTableLength = 8;
        cmdHeader[i].commandTableBaseAddress = baseAddr & 0xFFFFFFFF;
        cmdHeader[i].commandTableBaseAddressUpper = (baseAddr >> 32) & 0xFFFFFFFF;
        baseAddr += 256;
    }

    startCommandSending(port);
    return SUCCESS;
}

static struct disk* createDisk(HBA_PORT port, uint16_t id)
{
    struct disk* disk = kzalloc(sizeof(struct disk));
    RETNULL(disk);

    struct ahciPrivate* private = kzalloc(sizeof(struct ahciPrivate));
    RETNULL(private);

    private->port = port;

    disk->id = id;
    disk->type = DISK_TYPE_PHYSICAL;
    disk->driver = copyDriver(&driver);

    RETNULL(disk->driver);
    disk->driver->private = private;
    return disk;
}

static int scanForDisk(struct disk** diskList, int* diskFoundCount, uint16_t nextId)
{
    struct pciDevice** devices = getAllPciDevicesByClass(PCI_CLASS_MASS_STORAGE_CONTROLLER, PCI_SUBCLASS_MA_SATA_CONTROLLER, 0x1);

    int counter = 0;
    while (devices[counter] != NULL)
    {
        struct pciBarInfo* barInfo = getPciBarInfo(devices[counter], 5);
        counter++;

        if (barInfo == NULL || barInfo->isIo) continue;

        kprintf("  AHCI: Base: %x, size: %x\n", barInfo->base, barInfo->size);

        void* mapAddress = (void*)0xffffA00000000000;

        int ret = remapPhysicalToVirtualRange((void*)((uint64_t)barInfo->base), mapAddress, barInfo->size, getKernelPageTable());
        RETERROR(ret);

        kprintf("  Remapped: %x -> %x, size: %x\n", barInfo->base, mapAddress, barInfo->size);

        volatile struct hba_mem* hbaMem = mapAddress;
        if (hbaMem->ghc >> 31 != 1)
        {
            kprintf("  [WARN]: Found a AHCI controller. But the controller is not in AHCI-mode.\n");
            continue;
        }

        for (uint8_t i = 0; i < 32; i++)
        {
            if (((hbaMem->pi >> i) & 0x1) == 1)
            {
                //Port is physical present. But it does not mean there is something connected
                uint8_t deviceDetection = hbaMem->port[i].sataStatus & 0xF;
                uint8_t interfacePowerManagement = (hbaMem->port[i].sataStatus >> 8) & 0xF;

                if (deviceDetection == 0x3 && interfacePowerManagement == 0x1)
                {
                    //There is something connected and active
                    if (hbaMem->port[i].signature == 0x00000101)
                    {
                        kprintf("  SATA-Drive found at port: %u\n", i);
                        ret = remapPort(&hbaMem->port[i]);
                        RETERROR(ret);

                        struct disk* disk = createDisk(&hbaMem->port[i], nextId);

                        RETNULLERROR(disk, -ENMEM);

                        diskList[*diskFoundCount] = disk;
                        nextId++;
                        (*diskFoundCount)++;
                    }
                }
            }
        }
    }
    return SUCCESS;
}

static struct diskInfo ahciIdentifyCommand(void* private)
{
    struct diskInfo diskInfo;
    memset(&diskInfo, 0, sizeof(struct diskInfo));

    struct ahciPrivate* pr = private;

    //Just wait for now
    int slot = 0;
    do {
        slot = getFreeSlot(pr->port);
        if (slot > -1)break;
    }while (1);

    //VIRT TO PHYS

    //Setup command header
    volatile struct commandHeader* cmdHeader = (volatile struct commandHeader*)((uint64_t)pr->port->commandListBaseUpper << 32 | pr->port->commandListBase);

    cmdHeader[slot].commandFisLength = sizeof(struct fisHost2Device) / sizeof(uint32_t);
    cmdHeader[slot].write = 0; //We read
    cmdHeader[slot].physicalRegionDescriptorTableLength = 1;

    //Setup CommandTable
    volatile struct commandTable* cmdTable = (volatile struct commandTable*)((uint64_t)cmdHeader[slot].commandTableBaseAddressUpper << 32 | cmdHeader[slot].commandTableBaseAddress);

    void* dataBuffer = kzalloc(512);
    if (dataBuffer == NULL) return diskInfo;
    uint64_t dataBufferAddress = (uint64_t)virtualToPhysical(dataBuffer, getKernelPageTable());

    cmdTable->physicalRegionDescriptorTableEntries[0].dataBaseAddress = dataBufferAddress & 0xFFFFFFFF;
    cmdTable->physicalRegionDescriptorTableEntries[0].dataBaseAddressUpper = (dataBufferAddress >> 32) & 0xFFFFFFFF;
    cmdTable->physicalRegionDescriptorTableEntries[0].dataBaseCount = 512-1; //0-Indexed
    cmdTable->physicalRegionDescriptorTableEntries[0].interruptOnCompletion = 1;

    //Setup FIS
    struct fisHost2Device* fis = (struct fisHost2Device*)&cmdTable->commandFis;
    memset(fis, 0, sizeof(struct fisHost2Device));

    fis->fisType = FIS_TYPE_REG_H2D;
    fis->c = 1;
    fis->command = 0xEC; //ATA Identify Command

    //Send command
    while (pr->port->taskFileData & (0x80 | 0x08)){}
    pr->port->commandIssue |= (1 << slot);

    while (pr->port->commandIssue & (1 << slot)){} //I know we have interrupts activated, but pull this shit for now. (Just found out this bitch doesn't fire interrupts)

    //NOT FINISHED, WILL RETURN ZERO. IM JUST EXTREMELY TIRED FOR NOW ITS 5AM FUCK.
    return diskInfo;
}

struct diskDriver* registerAHCI()
{
    return &driver;
}
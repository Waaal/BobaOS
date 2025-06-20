#include "ahci.h"

#include <macros.h>
#include <stddef.h>
#include <memory/memory.h>
#include <memory/kheap/kheap.h>
#include <string/string.h>

#include "kernel.h"
#include "config.h"
#include "status.h"
#include "hardware/pci/pci.h"
#include "print.h"
#include "memory/paging/paging.h"

struct ahciPrivate
{
    HBA_PORT port;
    uint8_t availablePhysicalRegionDescriptorTableEntries;
};

static struct diskInfo ahciIdentifyCommand(void* private);
static int scanForDisk(struct disk** diskList, int* diskFoundCount, uint16_t nextId);
static int ahciRead(uint64_t lba, uint64_t total, void* out, void* private);
static int ahciWrite(uint64_t lba, uint64_t total, void* in, void* private);

static struct diskDriver driver = {
    .type = DISK_DRIVER_TYPE_AHCI,
    .scanForDisk = scanForDisk,
    .getInfo = ahciIdentifyCommand,
    .read = ahciRead,
    .write = ahciWrite
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
    uint64_t baseAddr = (uint64_t)addr;

    kprintf("    Map port commandHeader to: %x \n", baseAddr);

    port->interruptEnable = 0xFFFFFFFF; //Enable every interrupt

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
        cmdHeader[i].physicalRegionDescriptorTableLength = BOBAOS_MAX_AHCI_PRDT_ENTRIES;
        cmdHeader[i].commandTableBaseAddress = baseAddr & 0xFFFFFFFF;
        cmdHeader[i].commandTableBaseAddressUpper = (baseAddr >> 32) & 0xFFFFFFFF;
        baseAddr += sizeof(struct commandHeader);
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
    private->availablePhysicalRegionDescriptorTableEntries = BOBAOS_MAX_AHCI_PRDT_ENTRIES;

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

        hbaMem->ghc |= 2; //Enable global interrupts

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

                        //TODO:
                        /* Check the hbaMem->cap register bit 7 (SSC bit) if the controller supports multiple slot use at the same time. (NCQ)
                         * If this bit is set, we can use multiple slots per port. BUT THE HARDDISKS NEEDS TO SUPPROT THIS.
                         * To see if the harddisk supports this, we check in the IDENTIFY command the word 76 bit 8 for NCQ support.
                         * word 77 gives us the max que depth.
                         */
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

static void zeroCommandHeaderFlags(volatile struct commandHeader* cmdHeader)
{
    cmdHeader->commandFisLength = 0;
    cmdHeader->atapi = 0;
    cmdHeader->write = 0;
    cmdHeader->prefetchable = 0;
    cmdHeader->reset = 0;
    cmdHeader->bist = 0;
    cmdHeader->clearBusy = 0;
    cmdHeader->portMultiplierPort = 0;
    cmdHeader->physicalRegionDescriptorTableLength = 0;
    cmdHeader->physicalRegionDescriptorTableBytesTransferred = 0;
}

static struct diskInfo ahciIdentifyCommand(void* private)
{
    struct diskInfo diskInfo;
    memset(&diskInfo, 0, sizeof(struct diskInfo));

    struct ahciPrivate* pr = private;
    pr->port->interruptStatus = (uint32_t)-1; //Reset interruptStatus

    //Just wait for now
    int slot = 0;
    slot = getFreeSlot(pr->port);
    if (slot < 0){return diskInfo;} //ERROR

    //Setup command header
    volatile struct commandHeader* cmdHeader = (volatile struct commandHeader*)((uint64_t)pr->port->commandListBaseUpper >> 32 | pr->port->commandListBase);
    zeroCommandHeaderFlags(&cmdHeader[slot]);

    cmdHeader[slot].commandFisLength = sizeof(struct fisHost2Device) / sizeof(uint32_t);
    //cmdHeader[slot].write = 0; //We read
    cmdHeader[slot].physicalRegionDescriptorTableLength = 1;

    //Setup CommandTable
    volatile struct commandTable* cmdTable = (volatile struct commandTable*)((uint64_t)cmdHeader[slot].commandTableBaseAddressUpper >> 32 | cmdHeader[slot].commandTableBaseAddress);

    uint16_t* dataBuffer = kzalloc(512);
    if (dataBuffer == NULL) return diskInfo;
    uint64_t dataBufferAddress = (uint64_t)virtualToPhysical(dataBuffer, getKernelPageTable());

    cmdTable->physicalRegionDescriptorTableEntries[0].dataBaseAddress = dataBufferAddress & 0xFFFFFFFF;
    cmdTable->physicalRegionDescriptorTableEntries[0].dataBaseAddressUpper = (dataBufferAddress >> 32) & 0xFFFFFFFF;
    cmdTable->physicalRegionDescriptorTableEntries[0].dataBaseCount = 512-1; //0-Indexed
    cmdTable->physicalRegionDescriptorTableEntries[0].interruptOnCompletion = 1;

    //Setup FIS
    struct fisHost2Device* fis = (struct fisHost2Device*)&cmdTable->commandFis;
    memset(fis, 0x0, 64);

    fis->fisType = FIS_TYPE_REG_H2D;
    fis->c = 1;
    fis->command = 0xEC; //ATA Identify Command

    //Send command
    while (pr->port->taskFileData & (0x80 | 0x08)){}
    pr->port->commandIssue |= (1 << slot);

    while (pr->port->commandIssue & (1 << slot)){} //I know we have interrupts activated, but pull this shit for now. (Just found out this bitch doesn't fire interrupts)

    //Extract model string
    char model[42];
    for (uint8_t i = 0; i < 20; i++)
    {
        model[i*2] = (char)(dataBuffer[27+i] >> 8);
        model[i*2+1] = (char)(dataBuffer[27+i] & 0xFF);
    }
    model[41] = 0;

    //Extract total LBAs
    uint32_t totalLBA = 0;
    totalLBA = dataBuffer[60];
    totalLBA |= dataBuffer[61] << 16;

    //Fill diskinfo return struct
    strncpy(diskInfo.name, model, 42);
    diskInfo.size = totalLBA * 512;

    return diskInfo;
}

static int ahciRead(uint64_t lba, uint64_t total, void* out, void* private)
{
    struct ahciPrivate* pr = private;
    HBA_PORT port = pr->port;
    uint64_t totalDataRead = total*512;

    if (total > 0x3FFFFF)
    {
        //0x3FFFFF is the max 1 physicalRegionDescriptorTableEntry can hold. I know we have max 8 but im to tired for this now lol.
        //Just let it be like this
        return -ELONG;
    }

    port->interruptStatus = (uint32_t)-1; //Reset interruptStatus
    int slot = getFreeSlot(port);
    RETERROR(slot);

    //CmdHeader
    volatile struct commandHeader* cmdHeader = (volatile struct commandHeader*)((uint64_t)pr->port->commandListBaseUpper >> 32 | pr->port->commandListBase);
    zeroCommandHeaderFlags(&cmdHeader[slot]);
    cmdHeader[slot].commandFisLength = sizeof(struct fisHost2Device) / 4;
    cmdHeader[slot].physicalRegionDescriptorTableLength = 1; //For now

    //CmdTables
    volatile struct commandTable* cmdTable = (volatile struct commandTable*)((uint64_t)cmdHeader[slot].commandTableBaseAddressUpper >> 32 | cmdHeader[slot].commandTableBaseAddress);

    uint64_t dataBufferAddress = (uint64_t)virtualToPhysical(out, getKernelPageTable());
    cmdTable->physicalRegionDescriptorTableEntries[0].dataBaseAddress = (uint32_t)((uint64_t)dataBufferAddress);
    cmdTable->physicalRegionDescriptorTableEntries[0].dataBaseAddressUpper = (uint32_t)(((uint64_t)dataBufferAddress >> 32));
    cmdTable->physicalRegionDescriptorTableEntries[0].dataBaseCount = totalDataRead-1;
    cmdTable->physicalRegionDescriptorTableEntries[0].interruptOnCompletion = 1;

    //Fis Command
    struct fisHost2Device* fis = (struct fisHost2Device*)&cmdTable->commandFis;
    memset(fis, 0x0, 64);

    fis->fisType = FIS_TYPE_REG_H2D;
    fis->c = 1;
    fis->command = 0x25; //ATA_CMD_READ_DMA_EX

    fis->lba0 = (uint8_t)lba;
    fis->lba1 = (uint8_t)(lba >> 8);
    fis->lba2 = (uint8_t)(lba >> 16);
    fis->lba3 = (uint8_t)(lba >> 16);
    fis->lba4 = (uint8_t)(lba >> 32);
    fis->lba5 = (uint8_t)(lba >> 48);

    fis->device = 1<<6; //LBA mode
    fis->countLow = total & 0xFF;
    fis->countHigh = total>> 8 & 0xFF;

    while ((port->taskFileData & 0x88)){}

    pr->port->commandIssue |= (1 << slot); //We do the OR even tough we dont know if the port supports multiple slots at the same time.

    while (1)
    {
        if ((port->commandIssue & (1 << slot)) == 0) break;

        if (port->interruptStatus & (1 << 30))
        {
            //error
            return -EHARDWARE;
        }
    }
    return SUCCESS;
}

static int ahciWrite(uint64_t lba, uint64_t total, void* in, void* private)
{
    struct ahciPrivate* pr = private;
    HBA_PORT port = pr->port;
    uint64_t totalDataWrite = total*512;

    if (total > 0x3FFFFF)
    {
        //0x3FFFFF is the max 1 physicalRegionDescriptorTableEntry can hold. I know we have max 8 but im to tired for this now lol.
        //Just let it be like this
        return -ELONG;
    }

    port->interruptStatus = (uint32_t)-1; //Reset interruptStatus
    int slot = getFreeSlot(port);
    RETERROR(slot);

    //CmdHeader
    volatile struct commandHeader* cmdHeader = (volatile struct commandHeader*)((uint64_t)pr->port->commandListBaseUpper >> 32 | pr->port->commandListBase);
    zeroCommandHeaderFlags(&cmdHeader[slot]);
    cmdHeader[slot].commandFisLength = sizeof(struct fisHost2Device) / 4;
    cmdHeader[slot].write = 1;
    cmdHeader[slot].physicalRegionDescriptorTableLength = 1; //For now

    //CmdTables
    volatile struct commandTable* cmdTable = (volatile struct commandTable*)((uint64_t)cmdHeader[slot].commandTableBaseAddressUpper >> 32 | cmdHeader[slot].commandTableBaseAddress);

    uint64_t dataBufferAddress = (uint64_t)virtualToPhysical(in, getKernelPageTable());
    cmdTable->physicalRegionDescriptorTableEntries[0].dataBaseAddress = (uint32_t)((uint64_t)dataBufferAddress);
    cmdTable->physicalRegionDescriptorTableEntries[0].dataBaseAddressUpper = (uint32_t)(((uint64_t)dataBufferAddress >> 32));
    cmdTable->physicalRegionDescriptorTableEntries[0].dataBaseCount = totalDataWrite-1;
    cmdTable->physicalRegionDescriptorTableEntries[0].interruptOnCompletion = 1;

    //Fis Command
    struct fisHost2Device* fis = (struct fisHost2Device*)&cmdTable->commandFis;
    memset(fis, 0x0, 64);

    fis->fisType = FIS_TYPE_REG_H2D;
    fis->c = 1;
    fis->command = 0x35; //ATA_CMD_WRITE_DMA_EX

    fis->lba0 = (uint8_t)lba;
    fis->lba1 = (uint8_t)(lba >> 8);
    fis->lba2 = (uint8_t)(lba >> 16);
    fis->lba3 = (uint8_t)(lba >> 16);
    fis->lba4 = (uint8_t)(lba >> 32);
    fis->lba5 = (uint8_t)(lba >> 48);

    fis->device = 1<<6; //LBA mode
    fis->countLow = total & 0xFF;
    fis->countHigh = total>> 8 & 0xFF;

    while ((port->taskFileData & 0x88)){}

    pr->port->commandIssue |= (1 << slot); //We do the OR even tough we dont know if the port supports multiple slots at the same time.

    while (1)
    {
        if ((port->commandIssue & (1 << slot)) == 0) break;

        if (port->interruptStatus & (1 << 30))
        {
            //error
            return -EHARDWARE;
        }
    }
    return SUCCESS;
}

struct diskDriver* registerAHCI()
{
    return &driver;
}
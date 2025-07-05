#include "acpi.h"

#include <kernel.h>
#include <macros.h>
#include <status.h>
#include <memory/paging/paging.h>

#include "config.h"
#include "string/string.h"
#include "print.h"

static char signature[8] = "RSD PTR ";
static char rsdtSignature[4] = "RSDT";

static uint8_t tablesAvailable = 0;
static uint64_t tableAddressList[BOBAOS_MAX_ACPI_TABLES];

static uint64_t systemDescriptorTableAddress = 0;

static int64_t findRsdp()
{
    //search EBDA and BIOS area (16 byte boundary)
    for (uint64_t c = 0; c < 2; c++)
    {
        uint64_t size = 0x1000;
        uint64_t address = 0x80000;
        if (c)
        {
            address = 0xE0000;
            size = 0x1FFFF;
        }

        for (uint64_t i = 0; i < size; i+=16)
        {
            struct xsdp* rsdp = (struct xsdp*)(address+i);

            if (!strncmp(rsdp->signature, signature, 8))
            {
                //acpi version 1.0 (32bit)
                if (!rsdp->revision)
                    systemDescriptorTableAddress = (uint64_t)rsdp->rsdtAddress;
                else //acpi version > 1.0 (64 bit)
                    systemDescriptorTableAddress = rsdp->xsdtAddress;

                return SUCCESS;
            }
        }
    }
    return -ENFOUND;
}

int acpiInit()
{
    return (int)findRsdp();
}

int saveAcpiTables()
{
    struct memoryMap* acpiAddressMap = getMemoryMapForAddress(systemDescriptorTableAddress);
    RETNULLERROR(acpiAddressMap, -ENFOUND);

    uint64_t offset = systemDescriptorTableAddress - acpiAddressMap->address;
    offset += (acpiAddressMap->address % SIZE_4KB);
    RETERROR(remapPhysicalToVirtualRange((void*)downToPage(acpiAddressMap->address), (void*)BOBAOS_ACPI_ADDRESS, downToPage(acpiAddressMap->length),getKernelPageTable()));

    struct acpisdtHeader* header = (struct acpisdtHeader*)(BOBAOS_ACPI_ADDRESS+offset);
    if (strncmp(header->signature, rsdtSignature, 4) != 0)
        return -ENFOUND;

    uint8_t addressSize = header->revision > 1 ? 8 : 4; //32 or 64 bit
    tablesAvailable = (header->length - sizeof(struct acpisdtHeader)) / addressSize;

    if (addressSize == 4)
    {
        uint32_t* pointerToOtherSDTs = (uint32_t*)(BOBAOS_ACPI_ADDRESS+offset+sizeof(struct acpisdtHeader));
        for (uint8_t i = 0; i < tablesAvailable; i++)
            tableAddressList[i] = (uint64_t)pointerToOtherSDTs[i];
    }
    else
    {
        uint64_t* pointerToOtherSDTs = (uint64_t*)(BOBAOS_ACPI_ADDRESS+offset+sizeof(struct acpisdtHeader));
        for (uint8_t i = 0; i < tablesAvailable; i++)
            tableAddressList[i] = (uint64_t)pointerToOtherSDTs[i];
    }
    return SUCCESS;
}
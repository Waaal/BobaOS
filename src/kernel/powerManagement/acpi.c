#include "acpi.h"

#include <status.h>

#include "string/string.h"

static char signature[8] = "RSD PTR ";

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
                //acpi version 1.0 is not supported. I mean why version 1.0... its 2025 mf
                if (!rsdp->revision)
                    return -EWMODE;

                return SUCCESS;
            }
        }
    }
    return -ENFOUND;
}

int acpiInit()
{
    int64_t ret = findRsdp();
    return (int)ret;
}
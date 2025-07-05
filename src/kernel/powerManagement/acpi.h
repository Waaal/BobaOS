#ifndef ACPI_H
#define ACPI_H

#include <stdint.h>

struct xsdp
{
    char signature[8];
    uint8_t checksum;
    char oemId[6];
    uint8_t revision;
    uint32_t rsdtAddress; //Deprecated
    uint32_t length;
    uint64_t xsdtAddress;
    uint8_t extendedChecksum;
    uint8_t reserved[3];
} __attribute__ ((packed));

struct acpisdtHeader
{
    char signature[4];
    uint32_t length;
    uint8_t revision;
    uint8_t checksum;
    char oemId[6];
    char oemTableId[8];
    uint32_t oemRevision;
    uint32_t creatorId;
    uint32_t creatorRevision;
} __attribute__ ((packed));

int acpiInit();
int saveAcpiTables();

#endif
#ifndef AHCI_H
#define AHCI_H

#include <stdint.h>
#include "disk/diskDriver.h"

enum frameInformationStructure
{
    FIS_TYPE_REG_H2D = 0x27,
    FIS_TYPE_REG_D2H = 0x34,
    FIS_TYPE_DMA_ACT = 0x39,
    FIS_TYPE_DMA_SETUP = 0x41,
    FIS_TYPE_DATA = 0x46,
    FIS_TYPE_BIST = 0x58,
    FIS_TYPE_PIO_SETUP = 0x5F,
    FIS_TYPE_DEV_BITS = 0xA1
};

struct fisHost2Device
{
    //DW 0
    uint8_t fisType;

    uint8_t pmport:4;
    uint8_t reserved0:3;
    uint8_t c:1;

    uint8_t command;
    uint8_t featureLow;

    //DW 1
    uint8_t lba0;
    uint8_t lba1;
    uint8_t lba2;
    uint8_t device;

    //DW 2
    uint8_t lba3;
    uint8_t lba4;
    uint8_t lba5;
    uint8_t featureHigh;

    //DW 3
    uint8_t countLow;
    uint8_t countHigh;
    uint8_t icc;
    uint8_t control;

    //Dw 4
    uint32_t reserved1;
} __attribute__((packed));

struct hba_port {
    uint32_t commandListBase;
    uint32_t commandListBaseUpper;
    uint32_t fisBase;
    uint32_t fisBaseUpper;
    uint32_t interruptStatus;
    uint32_t interruptEnable;
    uint32_t commandAndStatus;
    uint32_t reserved0;
    uint32_t taskFileData;
    uint32_t signature;         //sig
    uint32_t sataStatus;        //ssts
    uint32_t sataController;
    uint32_t sataError;
    uint32_t sataActive;
    uint32_t commandIssue;
    uint32_t sataNotification;
    uint32_t fisBaseSwitchControl;
    uint32_t reserved1[11];
    uint32_t vendor[4];
} __attribute__((packed));

struct hba_mem {
    uint32_t cap;
    uint32_t ghc;
    uint32_t is;
    uint32_t pi;
    uint32_t vs;
    uint32_t ccc_ctl;
    uint32_t ccc_pts;
    uint32_t em_loc;
    uint32_t em_ctl;
    uint32_t cap2;
    uint32_t bohc;
    uint8_t reserved[116];
    uint8_t vendor[96];
    struct hba_port port[32];
} __attribute__((packed));

//(serial-ata-ahci-spec-rev1-3-1.pdf PAGE 47)
struct commandHeader
{
    //DW0
    uint8_t commandFisLength:5;
    uint8_t atapi:1;
    uint8_t write:1;
    uint8_t prefetchable:1;
    uint8_t reset:1;
    uint8_t bist:1;
    uint8_t clearBusy:1;
    uint8_t rsv0:1;
    uint8_t portMultiplierPort:4;
    uint16_t physicalRegionDescriptorTableLength;

    uint32_t physicalRegionDescriptorTableBytesTransferred; //DW1
    uint32_t commandTableBaseAddress; //DW2
    uint32_t commandTableBaseAddressUpper; //DW3
    uint32_t rsv1[4]; //DW4-7 (reserved)
} __attribute__((packed));

struct physicalRegionDescriptorTable
{
    uint32_t dataBaseAddress;
    uint32_t dataBaseAddressUpper;
    uint32_t reserved0;

    uint32_t dataBaseCount:22;
    uint32_t reserved1:9;
    uint32_t interruptOnCompletion:1;
} __attribute__((packed));

struct commandTable
{
    uint8_t commandFis[64];
    uint8_t ataPiCommand[16];
    uint8_t reserved[48];
    struct physicalRegionDescriptorTable physicalRegionDescriptorTableEntries[8]; //Can have up to 65535
} __attribute__((packed));

typedef volatile struct hba_port* HBA_PORT;

struct diskDriver* registerAHCI();

#endif

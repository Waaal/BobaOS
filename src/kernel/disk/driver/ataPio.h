#ifndef ATA_PIO_H
#define ATA_PIO_H

#include <stdint.h>

#include "disk/disk.h"
#include "disk/diskDriver.h"

#define ATAPIO_PRIMARY_SELECT 0xA0
#define ATAPIO_SECONDARY_SELECT 0xB0

#define ATAPIO_REG_ERROR 1
#define ATAPIO_REG_FEATURES 1
#define ATAPIO_REG_SECTOR 2
#define ATAPIO_REG_LBA0 3
#define ATAPIO_REG_LBA1 4
#define ATAPIO_REG_LBA2 5
#define ATAPIO_REG_DRIVESELECT 6
#define ATAPIO_REG_STATUS 7
#define ATAPIO_REG_COMMAND 7

#define ATAPIO_COMMAND_IDENTIFY 0xEC

#define ATAPIO_COMMAND_READ_SECTORS 0x20
#define ATAPIO_COMMAND_WRITE_SECTORS 0x30

#define ATAPIO_STATUS_ERR 0x1
#define ATAPIO_STATUS_DRQ 0x8
#define ATAPIO_STATUS_RDY 0x40
#define ATAPIO_STATUS_BSY 0x80

#define ATAPIO_DEVICECON_SRST 4

int ataPioProbePort(uint16_t port, uint16_t deviceCon);
int ataPioAttach(struct disk* disk, uint16_t base, uint16_t select, uint16_t deviceCon);
struct diskDriver* registerAtaPioDriver();

#endif

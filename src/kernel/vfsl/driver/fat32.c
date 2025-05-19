#include "fat32.h"

#include <stdint.h>
#include <stddef.h>
#include <memory/memory.h>
#include <memory/kheap/kheap.h>
#include <string/string.h>

#include "status.h"
#include "disk/disk.h"

struct masterBootRecord
{
    uint8_t reserved[3];
    uint8_t OEMIdentifier[8];
    uint16_t bytesPerSector;
    uint8_t sectorsPerCluster;
    uint16_t reservedSectors;
    uint8_t fatCopies;
    uint16_t rootEntries;
    uint16_t totalSectors;
    uint8_t mediaDescriptor;
    uint16_t sectorsPerFat12_16;
    uint16_t sectorsPerTrack;
    uint16_t heads;
    uint32_t hiddenSectors;
    uint32_t totalSectorsBig;
    uint32_t sectorsPerFat32;
    uint16_t flags;
    uint16_t fatVersion;
    uint32_t rootDirCluster;
    uint16_t infoCluster;
    uint16_t backupBootCluster;
    uint8_t reserved2[12];
    uint8_t driveNumber;
    uint8_t reserved3;
    uint8_t signature;
    uint32_t volumeId;
    char volumeLabel[11];
    char systemIdString[8];
} __attribute__ ((packed));

static int resolve(struct disk* disk);
struct fileSystem fs =
{
    .name = "FAT32",
    .resolve = resolve,
    .open = NULL,
    .close = NULL,
    .read = NULL,
    .write = NULL,
    .private = NULL
};

static int resolve(struct disk* disk)
{
    if (disk == NULL){ return -EIARG}

    unsigned char mbr[512];
    disk->driver->read(0, 1, mbr, disk->driver->private);

    if (strncmp(((struct masterBootRecord*)mbr)->systemIdString, "FAT32   ", 8) == 0)
    {
        return 1;
    }
    return 0;
}

struct fileSystem* insertIntoFileSystem()
{
    void* private = kzalloc(8);
    if (private == NULL){ return NULL; }

    fs.private = private;
    struct fileSystem* retFs = kzalloc(sizeof(struct fileSystem));
    if (retFs == NULL){ kzfree(private); return NULL; }

    memcpy(retFs, &fs, sizeof(struct fileSystem));
    return retFs;
}
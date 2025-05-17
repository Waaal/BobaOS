#include "virtualFilesystemLayer.h"

#include <stddef.h>
#include "status.h"

struct fileSystem* fileSystemList[BOBAOS_MAX_FILESYSTEMS];
struct file* openFiles[BOBAOS_MAX_OPEN_FILES];

uint8_t currentFileSystem = 0;

static int addFileSystem(struct fileSystem* fileSystem)
{
    if (currentFileSystem >= BOBAOS_MAX_FILESYSTEMS)
    {
        return -ENMEM;
    }

    if (fileSystem == NULL)
    {
        return -EIARG;
    }

    fileSystemList[currentFileSystem] = fileSystem;
    currentFileSystem++;

    return 0;
}

static int appendFileSystemToDisk(struct fileSystem* fileSystem, struct disk* disk)
{
    if (disk != NULL && fileSystem != NULL)
    {
        disk->fileSystem = fileSystem;
        return 0;
    }
    return -EIARG;
}

static int addStaticFileSystems()
{
    //here add the FAT32 filesystem that comes with the kernel
    addFileSystem(NULL);
    return 0;
}

int vfslInit()
{
    return 0;
    int ret = 0;
    ret = addStaticFileSystems();
    return ret;
}

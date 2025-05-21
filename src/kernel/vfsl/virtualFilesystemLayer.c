#include "virtualFilesystemLayer.h"

#include <stddef.h>
#include <memory/kheap/kheap.h>

#include "status.h"
#include "memory/memory.h"
#include "string/string.h"
#include "driver/fat32.h"

struct fileSystem* fileSystemList[BOBAOS_MAX_FILESYSTEMS];
struct fileListEntry openFiles[BOBAOS_MAX_OPEN_FILES];

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
    int ret = 0;
    ret = addFileSystem(insertIntoFileSystem());
    return ret;
}

static int resolveStaticFileSystems()
{
    struct disk** allDisks = diskGetAll();
    if (allDisks == NULL){return -ENFOUND;}

    while (*allDisks != NULL)
    {
        for (uint8_t i = 0; i < currentFileSystem; i++)
        {
            if (fileSystemList[i]->resolve(*allDisks))
            {
                if (appendFileSystemToDisk(fileSystemList[i]->attach(*allDisks), *allDisks) < 0)
                {
                    return -EIARG;
                }
                return SUCCESS;
            }
        }
        allDisks++;
    }

    return -ENFOUND;
}

int vfslInit()
{
    int ret = 0;

    memset(openFiles, 0, sizeof(openFiles));
    memset(fileSystemList, 0, sizeof(fileSystemList));

    ret = addStaticFileSystems();
    if (ret < 0){return ret;}

    resolveStaticFileSystems();
    return ret;
}

struct file* fopen(const char* path, const char* mode)
{
    return NULL;
}

// COULD NOT TEST THEM FOR NOW. JUST HOPE THEY WORK LATER
/*

int addOpenFile(struct file* file)
{
    uint64_t id = strHash(file->path) % BOBAOS_MAX_OPEN_FILES;

    if (openFiles[id].file == NULL)
    {
        openFiles[id].file = file;
        openFiles[id].next = NULL;
        openFiles[id].prev = NULL;
    }
    else
    {
        struct fileListEntry* currEntry = &openFiles[id];
        while (strcmp(currEntry->file->path, file->path) == 0 && currEntry->next != NULL)
        {
            currEntry = currEntry->next;
        }

        struct fileListEntry* newEntry = kzalloc(sizeof(struct fileListEntry));
        newEntry->file = file;

        newEntry->prev = currEntry;
        currEntry->next = newEntry;
    }

    return 0;
}

int removeOpenFile(struct file* file)
{
    return 0;
}
*/

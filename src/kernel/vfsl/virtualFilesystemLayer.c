#include "virtualFilesystemLayer.h"

#include <stddef.h>
#include <memory/kheap/kheap.h>

#include "macros.h"

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

static int checkIfFileIsOpen(struct file* file)
{
    RETNULLERROR(file, -EIARG);

    uint64_t id = strHash(file->path) % BOBAOS_MAX_OPEN_FILES;
    if (openFiles[id].file != NULL)
    {
        struct fileListEntry* entry = &openFiles[id];
        while (entry != NULL)
        {
            if (strcmp(file->path, entry->file->path) == 0)
            {
                return 1;
            }
            entry = entry->next;
        }
    }
    return 0;
}

static int addOpenFile(struct file* file)
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
        RETNULLERROR(newEntry, -ENMEM);

        newEntry->file = file;
        newEntry->prev = currEntry;

        currEntry->next = newEntry;
    }

    return 0;
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
    struct pathTracer* pathTracer = createPathTracer(path);
    RETNULL(pathTracer);

    struct disk* disk = diskGet(pathTracer->diskId);
    RETNULL(disk->fileSystem);

    struct file* file = disk->fileSystem->open(pathTracer, mode, disk->fileSystem->private);
    destroyPathTracer(pathTracer);
    RETNULL(file);

    if (!checkIfFileIsOpen(file))
    {
        if (addOpenFile(file) >= 0)
        {
            return file;
        }
    }

    kzfree(file);
    return NULL;
}

int fread(struct file* file, void* out, uint64_t size, uint64_t count)
{
    if (!checkIfFileIsOpen(file)){return -ENFOUND;}

    struct disk* disk = diskGet(file->diskId);
    RETNULLERROR(disk, -EIARG);

    if (size*count > file->size + file->position){return -EOF;}

    int ret = disk->fileSystem->read(out, size*count, file, disk->fileSystem->private);
    if (ret == 0)
    {
        file->position += (size*count);
    }

    return ret;
}
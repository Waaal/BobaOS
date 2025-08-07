#include "virtualFilesystemLayer.h"

#include <print.h>
#include <stddef.h>
#include <memory/kheap/kheap.h>

#include "disk/disk.h"
#include "macros.h"

#include "status.h"
#include "string/string.h"
#include "driver/fat32.h"
#include "memory/memory.h"

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

static int appendFileSystemToDisk(struct fileSystem* fileSystem, struct diskPartition* volume)
{
    if (volume != NULL && fileSystem != NULL)
    {
        volume->fileSystem = fileSystem;
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
    struct diskPartition** volumes = partitionGetAll();
    RETNULLERROR(volumes, -ENFOUND);

    while (*volumes != NULL)
    {
        for (uint16_t i = 0; i < currentFileSystem; i++)
        {
            if (fileSystemList[i]->resolve(*volumes))
            {
                if (appendFileSystemToDisk(fileSystemList[i]->attach(*volumes), *volumes) < 0)
                {
                    return -EIARG;
                }
                return SUCCESS;
            }
        }
        volumes++;
    }

    return -ENFOUND;
}

static uint8_t checkIfFileIsOpenString(char* path)
{
    uint64_t id = strHash(path) % BOBAOS_MAX_OPEN_FILES;
    if (openFiles[id].file != NULL)
    {
        struct fileListEntry* entry = &openFiles[id];
        while (entry->file != NULL)
        {
            if (strcmp(path, entry->file->path) == 0)
            {
                return 1;
            }
            entry = entry->next;
        }
    }
    return 0;
}

static uint8_t checkIfFileIsOpen(struct file* file)
{
    RETNULLERROR(file, 0);
    return checkIfFileIsOpenString(file->path);
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

static uint8_t checkMode(const char* mode)
{
    uint8_t ret = 0;
    while (*mode != 0x0)
    {
        switch (*mode)
        {
            case 'r':
                if ((ret & FILE_MODE_WRITE) == 0 && (ret & FILE_MODE_APPEND) == 0)
                {
                    ret |= FILE_MODE_READ;
                    break;
                }
                return 0;
            case 'w':
                if ((ret & FILE_MODE_READ) == 0)
                {
                    ret |= FILE_MODE_WRITE;
                    break;
                }
                return 0;
            case 'a':
                if ((ret & FILE_MODE_READ) == 0)
                {
                    ret |= FILE_MODE_APPEND;
                    break;
                }
                return 0;
            default:
                return 0;
        }
        mode++;
    }
    return ret;
}

static int closeFile(struct file* file)
{
    uint64_t id = strHash(file->path) % BOBAOS_MAX_OPEN_FILES;
    struct fileListEntry* currEntry = &openFiles[id];

    do {
        if (strcmp(currEntry->file->path, file->path) == 0)
        {
            if (currEntry->prev == NULL && currEntry->next == NULL)
            {
                currEntry->file = NULL;
            }
            else
            {
                currEntry->file = currEntry->next->file;
                currEntry->next = currEntry->next->next;
                currEntry->next->prev = currEntry;
            }
            return SUCCESS;
        }
    currEntry = currEntry->next;

    } while (currEntry != NULL);

    return -ENFOUND;
}

int vfslInit()
{
    int ret = 0;

    memset(openFiles, 0, sizeof(openFiles));
    memset(fileSystemList, 0, sizeof(fileSystemList));

    ret = addStaticFileSystems();
    if (ret < 0){return ret;}

    ret = resolveStaticFileSystems();
    return ret;
}

struct file* fopen(const char* path, const char* mode, int* oErrCode)
{
    *oErrCode = 0;

    if (checkIfFileIsOpenString((char*)path))
    {
        *oErrCode = -EIUSE;
        return NULL;
    }

    uint8_t m = checkMode(mode);
    if (!m)
    {
        *oErrCode = -EWMODE;
        return NULL;
    }

    struct pathTracer* pathTracer = createPathTracer(path, oErrCode);
    RETNULL(pathTracer);
    
    struct diskPartition* volume = partitionGetByVolumeLabel(pathTracer->label);

    struct file* file = volume->fileSystem->open(pathTracer, (((m & FILE_MODE_WRITE) > 0) && ((m & FILE_MODE_APPEND) == 0) ? 1 : 0), volume->fileSystem->private, oErrCode);
    destroyPathTracer(pathTracer);
    RETNULL(file); //ErrorCode set by fileSystem->open

    file->mode = m;

    *oErrCode = addOpenFile(file);
    if (*oErrCode >= 0)
    {
        return file;
    }

    kzfree(file);
    return NULL;
}

int fseek(struct file* file, uint64_t offset)
{
    if (offset <= file->size)
    {
        file->position = offset;
        return SUCCESS;
    }
    return -ENMEM;
}

int fread(struct file* file, void* out, uint64_t size, uint64_t count)
{
    if (!checkIfFileIsOpen(file)){return -ENFOUND;}
    if ((file->mode & FILE_MODE_READ) == 0){return -EWMODE;}
    
    struct diskPartition* volume = partitionGetByVolumeLabel(file->label);
    RETNULLERROR(volume, -EIARG);

    if (size*count > file->size + file->position){return -EOF;}

    int ret = volume->fileSystem->read(out, size*count, file, volume->fileSystem->private);
    if (ret == 0)
    {
        file->position += (size*count);
    }
    return ret;
}

int fwrite(struct file* file, const void* in, uint64_t size, uint64_t count)
{
    if (!checkIfFileIsOpen(file)){return -ENFOUND;}
    if ((file->mode & FILE_MODE_WRITE) == 0){return -EWMODE;}

    struct diskPartition* volume = partitionGetByVolumeLabel(file->label); 
    RETNULLERROR(volume, -EIARG);

    int ret = volume->fileSystem->write(in, size*count, file, volume->fileSystem->private);
    if (ret == 0)
    {
        if ((file->mode & FILE_MODE_APPEND) > 0)
        {
            file->position += (size*count);
            file->size += (size*count);
        }
        else
        {
            file->size = file->position + (size*count);
            file->position += (size*count);
        }
    }
    return ret;
}

int fclose(struct file* file)
{
    if (!checkIfFileIsOpen(file)){return -ENFOUND;}
    return closeFile(file);
}

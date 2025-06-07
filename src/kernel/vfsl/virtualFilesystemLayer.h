#ifndef VIRTUALFILESYSTEMLAYER_H
#define VIRTUALFILESYSTEMLAYER_H

#include <stdint.h>

#include "config.h"
#include "disk/disk.h"
#include "pathTracer.h"

#define FILE_MODE_READ 1
#define FILE_MODE_WRITE 2
#define FILE_MODE_APPEND 4

struct file
{
    uint16_t diskId;
    char path[BOBAOS_MAX_PATH_SIZE];
    uint8_t mode;
    uint64_t position;
    uint64_t size;
};

//This linkedList is for the openFiles list.
//As the openFiles list uses a hash fuction. To store its open files.
//But if 2 or more different files have the same hash, the BOBAOS_MAX_OPEN_FILES is small,
//then use this linked list to store same hashes but different files
struct fileListEntry
{
    struct file* file;
    struct fileListEntry* next;
    struct fileListEntry* prev;
};

struct disk;
typedef int (*RESOLVE)(struct disk* disk);
typedef struct fileSystem* (*ATTACH_CALLBACK)(struct disk* disk);
typedef struct file* (*OPEN_FILE)(struct pathTracer* tracer, uint8_t create, void* private, int* oErrCode);
typedef int (*READ_FILE)(void* ptr, uint64_t size, struct file* file, void* private);
typedef int (*WRITE_FILE)(const void* ptr, uint64_t size, struct file* file, void* private);

struct fileSystem
{
    char name[32];
    RESOLVE resolve;
    ATTACH_CALLBACK attach;
    OPEN_FILE open;
    READ_FILE read;
    WRITE_FILE write;
    void* private;
};

int vfslInit();
struct file* fopen(const char* path, const char* mode, int* oErrCode);
int fread(struct file* file, void* out, uint64_t size, uint64_t count);
int fseek(struct file* file, uint64_t offset);
int fwrite(struct file* file, const void* in, uint64_t size, uint64_t count);
int fclose(struct file* file);

#endif

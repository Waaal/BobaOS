#ifndef VIRTUALFILESYSTEMLAYER_H
#define VIRTUALFILESYSTEMLAYER_H

#include <stdint.h>

#include "config.h"
#include "disk/disk.h"

#define FILE_OPENING_MODE_RB "rb"
#define FILE_OPENING_MODE_WB "wb"

struct file
{
    char path[BOBAOS_MAX_PATH_SIZE];
    uint64_t size;
};

//This linkedList is for the openFiles list.
//As the openFiles list uses a hash fuction. To store its open files.
//But if 2 or more different files have the same hash, the BOBAOS_MAX_OPEN_FILES is small,
//then use this linked list to store same hashes but different files
struct fileListEntry
{
    struct file* file;
    struct file* next;
    struct file* prev;
};

struct disk;
typedef int (*RESOLVE)(struct disk* disk);
typedef struct file* (*OPEN_FILE)(const char* path, const char* mode);
typedef int (*READ_FILE)(void* ptr, uint64_t size, struct file* file);
typedef int (*WRITE_FILE)(void* ptr, uint64_t size, struct file* file);
typedef int (*CLOSE_FILE)(struct file* file);

struct fileSystem
{
    char name[32];
    RESOLVE resolve;
    OPEN_FILE open;
    READ_FILE read;
    WRITE_FILE write;
    CLOSE_FILE close;
    void* private;
};

int vfslInit();
/*
int addOpenFile(struct file* file);
int removeOpenFile(struct file* file);
*/
#endif

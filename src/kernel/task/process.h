#ifndef PROCESS_H
#define PROCESS_H

#include <stdint.h>

#include "config.h"
#include "task.h"

enum processFormat
{
    PROCESS_FORMAT_BINARY,
    PROCESS_FORMAT_ELF
};

enum processType
{
    PROCESS_TYPE_USER,
    PROCESS_TYPE_KERNEL
};

enum processState
{
    PROCESS_STATE_RUNNING,
    PROCESS_STATE_DIED
};

struct processAllocation
{
    void* address;
    void* size;
    struct processAllocation* next;
};

struct process
{
    uint32_t pid;
    char path[BOBAOS_MAX_PATH_SIZE];
    struct task* mainTask;
    enum processFormat format;
    enum processType type;
    enum processState state;
    struct processAllocation* allocations;
    uint16_t flags;
    int8_t returnCode;
};

void processInit();
struct process* createProcess(const char* path, enum processType type, int* oErrCode);
int runProcess(struct process* process);

#endif

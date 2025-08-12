#ifndef PROCESS_H
#define PROCESS_H

#include <stdint.h>
#include <stddef.h>

#include "config.h"
#include "task.h"
#include "memory/paging/paging.h"

#define NO_PARENT_PROCESS NULL

#define PROCESS_FLAG_FORMAT_BINARY 1
#define PROCESS_FLAG_FORMAT_ELF 2
#define PROCESS_FLAG_TYPE_USER 4
#define PROCESS_FLAG_TYPE_KERNEL 8
#define PROCESS_FLAG_STATE_ALIVE 16
#define PROCESS_FLAG_STATE_DIED 32

enum processType
{
    PROCESS_TYPE_USER = 0x4,
    PROCESS_TYPER_KERNEL = 0x8
};

struct processAllocation
{
    void* address;
    void* size;
    struct processAllocation* next;
};

typedef struct process* PROCESS;
struct process
{
    uint32_t pid;
	PROCESS parentProcess;
    char path[BOBAOS_MAX_PATH_SIZE];
    TASK mainTask;
    uint32_t flags;
    struct processAllocation* allocations;
	PML4Table pageTable;
    int returnCode;
};

void processInit();
PROCESS createProcess(const char* path, PROCESS parentProcess, enum processType processType, int* oErrCode);
int runProcess(PROCESS process);

#endif

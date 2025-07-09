#include "process.h"

#include <stddef.h>

#include <status.h>
#include <macros.h>
#include <memory/memory.h>
#include <memory/kheap/kheap.h>

uint32_t nextProcess = 0;
uint32_t nextProcessId = 0;
static struct process processList[BOBAOS_MAX_PROCESSES];

static int addToProcessList(struct process* process)
{
    if (nextProcess >= BOBAOS_MAX_PROCESSES) return -ENSPACE;
    processList[nextProcess++] = *process;
    return SUCCESS;
}

void processInit()
{
    memset(processList, 0x0, sizeof(processList));
}

struct process* createProcess(const char* path, enum processType type, int* oErrCode)
{
    RETNULLSETERROR(path, -EIARG, oErrCode);

    struct process* process = kzalloc(sizeof(struct process));
    RETNULLSETERROR(process, -ENMEM, oErrCode);

    struct pathTracer* tracer = createPathTracer(path, oErrCode);
    GOTOERRORNULL(tracer, error);

    struct task* task = createTaskFromFile(tracer, oErrCode);
    GOTOERRORNULL(task, error);

    process->pid = nextProcessId++;
    process->type = type;
    process->mainTask = task;
    process->format = PROCESS_FORMAT_BINARY;

    *oErrCode = addToProcessList(process);
    if (*oErrCode < 0) goto error;

    destroyPathTracer(tracer);
    return process;

    error:
    destroyPathTracer(tracer);

    if (process != NULL)
        kzfree(process);
    if (task != NULL)
        kzfree(task);

    return NULL;
}
int runProcess(struct process* process)
{
    RETNULLERROR(process, -EIARG);
    return SUCCESS;
}
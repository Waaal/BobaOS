#include "process.h"

#include <stddef.h>

#include "config.h"
#include "memory/paging/paging.h"
#include "status.h"
#include "macros.h"
#include "memory/memory.h"
#include "memory/kheap/kheap.h"
#include "string/string.h"

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

PROCESS createProcess(const char* path, PROCESS parentProcess, uint8_t processType, int* oErrCode)
{
	if(processType == PROCESS_FLAG_TYPE_KERNEL)
	{
		*oErrCode = -ENIMPLEMENTED;
		return NULL;
	}

    PROCESS process = kzalloc(sizeof(struct process));
    RETNULLSETERROR(process, -ENMEM, oErrCode);
	
	uint32_t newProcessId = nextProcessId++;
	
    TASK task = createTaskFromFile(path, newProcessId, oErrCode);
    GOTOERRORNULL(task, error);
	
	PML4Table table = createEmptyPageTable();
	if(table == NULL)
	{
		*oErrCode = -ENMEM;
		goto error;
	}

    process->pid = newProcessId;
    process->mainTask = task;
	process->flags = (PROCESS_FLAG_FORMAT_BINARY | PROCESS_FLAG_STATE_ALIVE | processType);
	process->parentProcess = parentProcess;
	process->pageTable = table;
	
	strncpy(process->path, path, BOBAOS_MAX_PATH_SIZE);

    *oErrCode = addToProcessList(process);
    if (*oErrCode < 0) goto error;

    return process;

    error:
	nextProcessId--;
    if (process != NULL)
        kzfree(process);
    if (task != NULL)
        kzfree(task);
    return NULL;
}
int runProcess(PROCESS process)
{
    RETNULLERROR(process, -EIARG);
    //place all tasks from the process in the scheduler
	return SUCCESS;
}

#include "task.h"

#include <stdint.h>
#include <stddef.h>

#include "config.h"
#include "idt/idt.h"
#include "macros.h"
#include "status.h"
#include "vfsl/virtualFilesystemLayer.h"
#include "memory/kheap/kheap.h"

static uint32_t nextTaskId = 0;

static struct trapFrame* createNewTaskFrame(uint64_t taskStack, uint64_t taskEntry, int* oErrCode)
{
	struct trapFrame* frame = kzalloc(sizeof(struct trapFrame*));
	RETNULLSETERROR(frame, -ENMEM, oErrCode);
	
	frame->rsp = taskStack;
	frame->rip = taskEntry;
	frame->cs = BOBAOS_USER_SELECTOR_CODE;
	frame->ds = BOBAOS_USER_SELECTOR_DATA;

	return NULL;	
}

static TASK createTaskFromBinFile(const char* path, uint32_t pid, int* oErrCode)
{
	struct file* file = fopen(path, "r", oErrCode);
	RETNULL(file);	
	
	TASK task = kzalloc(sizeof(struct task));
	RETNULLSETERROR(task, -ENMEM, oErrCode);
	
	struct trapFrame* frame = createNewTaskFrame(BOBAOS_DEFAULT_TASK_STACK, BOBAOS_DEFAULT_PROCESS_ENTRY, oErrCode);
	RETNULL(frame);
		
	task->id = nextTaskId++;
	task->pid = pid;
	task->state = TASK_STATE_READY;
	task->kernelStack = (void*)BOBAOS_DEFAULT_TASK_KERNEL_STACK;
	task->frame = frame;
	return task;
}

TASK createTaskFromFile(const char* path, uint32_t pid, int* oErrCode)
{
    return createTaskFromBinFile(path, pid, oErrCode);

}

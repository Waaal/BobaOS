#ifndef TASK_H
#define TASK_H

#include <stdint.h>

enum taskState
{
	TASK_STATE_READY,
    TASK_STATE_RUNNING,
    TASK_STATE_SLEEP,
    TASK_STATE_KERNEL
};


typedef struct task* TASK;
struct task
{
    uint32_t id;
    uint32_t pid;
	struct trapFrame *frame;
    enum taskState state;
    void* kernelStack;

	TASK head;
	TASK tail;
};

TASK createTaskFromFile(const char* path, uint32_t pid, int* oErrCode);

#endif

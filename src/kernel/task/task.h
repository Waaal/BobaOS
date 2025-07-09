#ifndef TASK_H
#define TASK_H

#include "process.h"
#include "idt/idt.h"
#include "vfsl/pathTracer.h"

enum taskState
{
    TASK_STATE_RUNNING,
    TASK_STATE_SLEEP,
    TASK_STATE_KERNEL
};

struct task
{
    uint32_t id;
    struct process *process;
    struct trapframe *frame;
    enum taskState state;
    void* entry;
    void* stack;
    void* kernelStack;
};

struct task* createTaskFromFile(struct pathTracer* tracer, int* oErrCode);

#endif

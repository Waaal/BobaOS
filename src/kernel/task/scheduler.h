#ifndef SCHEDULER_H
#define SCHEDULER_H

#include "task.h"

struct taskList
{
    struct task* head;
    struct task* tail;
};

int addTask(struct task* task);
int removeTask(struct task* task);
int runScheduler();
void yield();

#endif

#ifndef SCHEDULER_H
#define SCHEDULER_H

#include "task.h"

int addTask(struct task* task);
int removeTask(struct task* task);
int runScheduler();
void yield();

#endif

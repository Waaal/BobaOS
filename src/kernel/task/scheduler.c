#include "scheduler.h"

#include <config.h>
#include <status.h>

static struct taskList* taskList;

int runScheduler()
{
    return SUCCESS;
}

int addTask(struct task* task)
{
    if (task && taskList){}
    return SUCCESS;
}

void yield()
{

}

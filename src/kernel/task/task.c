#include "task.h"

#include <stdint.h>
#include <stddef.h>

#include <macros.h>
#include <status.h>

struct task* createTaskFromFile(struct pathTracer* tracer, int* oErrCode)
{
    RETNULLSETERROR(tracer, -EIARG, oErrCode);
    return NULL;
}

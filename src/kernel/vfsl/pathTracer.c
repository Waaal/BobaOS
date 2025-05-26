#include "pathTracer.h"

#include <stddef.h>

#include "status.h"
#include "macros.h"
#include "memory/kheap/kheap.h"
#include "string/string.h"
#include "disk/disk.h"

void destroyPathTracer(struct pathTracer* tracer)
{
    if (tracer == NULL) {return;}
    struct pathTracerPart* temp = tracer->root;
    while (temp != NULL)
    {
        struct pathTracerPart* temp2 = temp;
        temp = temp2->next;
        kzfree(temp2);
    }

    kzfree(tracer);
}

struct pathTracer* createPathTracer(const char* path)
{
    if (path == NULL) {return NULL;}
    if (!isNumber(path[0])){return NULL;}

    uint64_t strLen = strlen(path);
    if (strLen >= BOBAOS_MAX_PATH_SIZE){return NULL;}

    uint8_t diskId = toNumber(path[0]);

    struct disk* disk = diskGet(diskId);
    RETNULL(disk);

    struct pathTracer* pathTracer = kzalloc(sizeof(struct pathTracer));
    RETNULL(pathTracer);

    pathTracer->diskId = diskId;

    struct pathTracerPart* part = kzalloc(sizeof(struct pathTracerPart));
    if (part == NULL){ kzfree(pathTracer); return NULL;}

    pathTracer->root = part;

    char pathPart[BOBAOS_MAX_PATH_SIZE];
    uint8_t pathPartCounter = 0;

    for (uint64_t i = 2; i < strLen; i++)
    {
        if (path[i] == '/')
        {
            pathPart[pathPartCounter] = 0x0;
            strcpy(part->pathPart, pathPart);
            struct pathTracerPart* tempPath = kzalloc(sizeof(struct pathTracerPart));
            if (tempPath == NULL){ goto free; }

            part->type = PATH_TRACER_PART_DIRECTORY;
            part->next = tempPath;
            tempPath->prev = part;

            part = tempPath;
            pathPartCounter = 0;
            continue;
        }
        pathPart[pathPartCounter] = path[i];
        pathPartCounter++;
    }

    if (pathPartCounter == 0)
    {
        //Nothing more path had no file just a directory
        if (part->prev != NULL)
            part->prev->next = NULL;
        kzfree(part);
    }
    else
    {
        pathPart[pathPartCounter] = 0x0;
        strcpy(part->pathPart, pathPart);
        part->type = PATH_TRACER_PART_FILE;
    }
    return pathTracer;

    free:
    destroyPathTracer(pathTracer);
    return NULL;
}

struct pathTracerPart* pathTracerStartTrace(struct pathTracer* tracer)
{
    if (tracer == NULL || tracer->root == NULL){return NULL;}
    return tracer->root;
}

struct pathTracerPart* pathTracerGetNext(struct pathTracerPart* tracer)
{
    if (tracer == NULL || tracer->next == NULL){return NULL;}
    return tracer->next;
}

char* pathTracerGetPathString(struct pathTracer* tracer)
{
    char* ret = kzalloc(BOBAOS_MAX_PATH_SIZE);
    RETNULL(ret);

    ret[0] = (char)(tracer->diskId + '0');
    ret[1] = ':';

    struct pathTracerPart* part = pathTracerStartTrace(tracer);
    uint16_t counter = 2;
    while (part != NULL)
    {
        strncpy(ret+counter, part->pathPart, strlen(part->pathPart));
        part = pathTracerGetNext(part);

        counter+= strlen(part->pathPart);
        *(ret+counter) += '/';
        counter++;
    }
    return ret;
}

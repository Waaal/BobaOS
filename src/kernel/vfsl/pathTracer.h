#ifndef PATHTRACER_H
#define PATHTRACER_H

#include <config.h>
#include <stdint.h>

#define PATH_TRACER_PART_DIRECTORY 1
#define PATH_TRACER_PART_FILE 2
typedef uint8_t PATH_TRACER_TYPE;

struct pathTracer
{
    uint64_t diskId;
    struct pathTracerPart* root;
};

struct pathTracerPart
{
    char pathPart[BOBAOS_MAX_PATH_SIZE];
    PATH_TRACER_TYPE type;
    struct pathTracerPart *prev;
    struct pathTracerPart *next;
};

struct pathTracer* createPathTracer(const char* path);
void destroyPathTracer(struct pathTracer* tracer);
struct pathTracerPart* pathTracerStartTrace(struct pathTracer* tracer);
struct pathTracerPart* pathTracerGetNext(struct pathTracerPart* tracer);
char* pathTracerGetPathString(struct pathTracer* tracer);

#endif

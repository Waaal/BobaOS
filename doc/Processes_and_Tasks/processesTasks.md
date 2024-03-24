# Processes and Tasks

## Differnece between Process and Tasks
A process is a application running on the system. It is called a process while it is running. A Task is the actual thing that gets executet. A process can have many tasks. A task is basically a thread.

## Process
As we clarified a process is the actuall programm running. So a Process has a process ID, the file path from which the code got loaded, at least one taks which is the main taks and the kernel should also keep track of all the allocations the process has done with malloc. So if the process crashes or closes and the process didnt clean up all of its allocations the kernel can resolve them.

### Example structure of a simple process
``` c
struct process
{
    uint16_t id;
    char filename[BOBAOS_MAX_PATH];

    //The main process task
    struct task* task;

    //This allocations holds all the memory requested by the process
    //so we can free all the allocations if the process gets closed and the process didnt free them themself
    //(In a better implemenation this is not done with a array but with a pointer and dynamic memory but to keep it simple its a array)
    void* allocations[BOBAOS_MAX_PROGRAM_ALLOCATIONS];

    //The physical pointer to the process memory
    void* ptr;

    //The physical poitner to the stack
    void* stack;

    //The size of the data pointed to by "ptr"
    uint32_t size;
};
```

## Taks

### Exaple strucure of a simple task
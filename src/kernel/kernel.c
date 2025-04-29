#include "kernel.h"

#include <stddef.h>

#include "terminal.h"
#include "print.h"
#include "memory/kheap/kheap.h"
#include "memory/kheap/kheap_buddy.h"
#include "memory/paging/paging.h"
#include "gdt/gdt.h"
#include "koal/koal.h"

void panic(enum panicType type, struct trapFrame* frame, const char* message)
{
	disableInterrupts();

	terminalClear(0x1);
	print("KERNEL PANIC\n\n");	

	switch(type)
	{
		case PANIC_TYPE_KERNEL:
			print("Panic Type: KERNEL\n");
			break;
		case PANIC_TYPE_EXCEPTION:
			print("Panic Type: EXCEPTION\n");

			if(frame != NULL)
			{
				printTrapFrame(frame);
			}
			break;
		default:
			print("Panic Type: UNKNOWN\n");
			break;
	}
	
	kprintf("Panic message: %s", message);
	while(1){}
}

struct gdt gdt[] = {
	{ .limit = 0x0, .base = 0x0, .access = 0x0,  .flags = 0x0 }, // NULL
	{ .limit = 0x0, .base = 0x0, .access = 0x9A, .flags = 0x2 }, //	K CODE
	{ .limit = 0x0, .base = 0x0, .access = 0x92, .flags = 0x2 }  // K DATA
};

void kmain()
{
	loadGdt(gdt);
	
	koalInit();
	terminalInit();
	koalSelectCurrentOutputByName("TEXT_TERMINAL");

	print("Hello World\n\n");
	
	idtInit();
	enableInterrupts();
	
	readMemoryMap();	
	kprintf("Memory\n  Available memory: %x\n  Available upper memory: %x\n\n", getMaxMemorySize(), getUpperMemorySize());
	
	if(kheapBInit() < 0)
	{
		panic(PANIC_TYPE_KERNEL, NULL, "Not enough memory to initialize Kernel Heap");
	}
	
	void* test1 = kzBalloc(3000);
	void* test2 = kzBalloc(3000);

	void* test3 = kzBalloc(3000);
	void* test4 = kzBalloc(3000);

	if(test1 && test2 && test3 && test4){}

	while(1){}

	if(kheapInit() < 0)
	{
		panic(PANIC_TYPE_KERNEL, NULL, "Not enough memory to initialize Kernel Heap");
	}	
	
	PML4Table kernelPageTable = createKernelTable(0x0, 0x0, getMaxMemorySize());
	if(kernelPageTable == NULL)
	{
		panic(PANIC_TYPE_KERNEL, NULL, "Not enough memory for Kernel");
	}	

	while(1){}
}

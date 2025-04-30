#include "kernel.h"

#include <stddef.h>

#include "config.h"
#include "version.h"

#include "terminal.h"
#include "print.h"
#include "memory/kheap/kheap.h"
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

	kprintf("BobaOS v%u.%u.%u - Milk Tea\n\n", KERNEL_VERSION_MAJOR, KERNEL_VERSION_MINOR, KERNEL_VERSION_PATCH);
	
	idtInit();
	enableInterrupts();

	readMemoryMap();	
	kprintf("Memory\n  Available memory: %x\n  Available upper memory: %x\n\n", getMaxMemorySize(), getUpperMemorySize());

#if BOBAOS_USE_BUDDY_FOR_KERNEL_HEAP == 1
	if(kheapInit(KERNEL_HEAP_TYPE_BUDDY) < 0)
	{
		panic(PANIC_TYPE_KERNEL, NULL, "Not enough memory to initialize Kernel buddy Heap");
	}	
#else
	if(kheapInit(KERNEL_HEAP_TYPE_PAGE) < 0)
	{
		panic(PANIC_TYPE_KERNEL, NULL, "Not enough memory to initialize Kernel page Heap");
	}	
#endif
	
	PML4Table kernelPageTable = createKernelTable(0x0, 0x0, getMaxMemorySize());
	if(kernelPageTable == NULL)
	{
		panic(PANIC_TYPE_KERNEL, NULL, "Not enough memory for Kernel");
	}	
	
	void* test1 = kzalloc(1);
	void* test2 = kzalloc(2);

	if(test1 && test2){}

	while(1){}
}

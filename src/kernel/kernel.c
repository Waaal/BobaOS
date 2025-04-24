#include "kernel.h"

#include <stddef.h>

#include "terminal.h"
#include "memory/kheap/kheap.h"
#include "memory/paging/paging.h"
#include "gdt/gdt.h"

void panic(enum panicType type, struct trapFrame* frame, const char* message)
{
	disableInterrupts();

	terminalClear(0x1);
	terminalPrint("KERNEL PANIC/n/n");	

	switch(type)
	{
		case PANIC_TYPE_KERNEL:
			terminalPrint("Panic Type: KERNEL/n");
			break;
		case PANIC_TYPE_EXCEPTION:
			terminalPrint("Panic Type: EXCEPTION/n");

			if(frame != NULL)
			{
				printTrapFrame(frame);
			}
			break;
		default:
			terminalPrint("Panic Type: UNKNOWN/n");
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
	
	terminalInit();
	terminalPrint("Hello World/n");
	
	kprintf("Hello number:  %u/n", 123456789);
	kprintf("Hello hex:     %x/n", 123456789);
	kprintf("Hello string   %s/n/n", "Im a string!");

	kheap_init();	
	
	PLM4Table kernelPageTable = createKernelTable(0x0, 0x0, 0x100000000);
	if(kernelPageTable){}	
	
	idtInit();
	enableInterrupts();	
	
	panic(PANIC_TYPE_KERNEL, NULL, "test panic");

	while(1){}
}

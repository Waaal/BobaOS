#include "kernel.h"

#include <stdint.h>

#include "terminal.h"
#include "memory/kheap/kheap.h"
#include "memory/paging/paging.h"
#include "gdt/gdt.h"
#include "idt/idt.h"

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

	while(1){}
}

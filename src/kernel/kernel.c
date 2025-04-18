#include "kernel.h"

#include <stdint.h>

#include "terminal.h"
#include "memory/kheap/kheap.h"
#include "memory/paging/paging.h"
#include "gdt/gdt.h"

struct gdt gdt[] = {
	{ .limit = 0x0, .base = 0x0, .access = 0x0,  .flags = 0x0 }, // NULL
	{ .limit = 0x0, .base = 0x0, .access = 0x98, .flags = 0x2 }, //	K CODE
	{ .limit = 0x0, .base = 0x0, .access = 0x90, .flags = 0x2 }  // K DATA
};

void kmain()
{
	loadGdt(gdt);
	
	terminalInit();
	terminalPrint("Hello World/n");
	
	kheap_init();	

	PLM4Table kernelPageTable = createKernelTable(0x0, 0x0, 0x100000000);
	
	uint8_t* test1 = kzalloc(1);
	uint8_t* test2 = kzalloc(1);

	*test1 = 100;
	*test2 = 200;

	remapPage(test1, test2, kernelPageTable);
	*test1 = 5;

	while(1){}
}

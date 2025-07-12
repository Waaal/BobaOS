#include "kernel.h"

#include <macros.h>
#include <status.h>
#include <stddef.h>
#include <memory/memory.h>
#include <string/string.h>

#include "config.h"
#include "version.h"

#include "terminal.h"
#include "print.h"
#include "memory/kheap/kheap.h"
#include "gdt/gdt.h"
#include "koal/koal.h"
#include "hardware/pci/pci.h"

#include "memory/mmioEngine.h"
#include "disk/disk.h"
#include "disk/diskDriver.h"
#include "disk/stream.h"
#include "vfsl/virtualFilesystemLayer.h"
#include "vfsl/pathTracer.h"
#include "task/tss.h"
#include "powerManagement/acpi.h"
#include "task/process.h"

static PML4Table kernelPageTable;

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
	{ .limit = 0x0, .base = 0x0, .access = 0x92, .flags = 0x2 }, // K DATA
	{ .limit = sizeof(struct tss)-1, .base = 0x0, 0x89, 0x0} //TSS entry
};

void kmain()
{
	gdt[3].base = getTssAddress();
	loadGdt(gdt);

	initTss();
	koalInit();
	terminalInit();
	koalSelectCurrentOutputByName("TEXT_TERMINAL");

	kprintf("BobaOS v%u.%u.%u - Tapioca Core\n\n", KERNEL_VERSION_MAJOR, KERNEL_VERSION_MINOR, KERNEL_VERSION_PATCH);

	idtInit();
	enableInterrupts();

	int ret = acpiInit();
	if (ret < 0)
	{
		if (ret == -EWMODE)
			panic(PANIC_TYPE_KERNEL, NULL,"ACPI v1.0 is not supported. At least v2.0 is required.");
		else
			panic(PANIC_TYPE_KERNEL, NULL,"ACPI not found.");
	}

	mmioEngineInit();

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
	
	kernelPageTable = createKernelTable(0x0);
	if(kernelPageTable == NULL)
	{
		panic(PANIC_TYPE_KERNEL, NULL, "Not enough kernel heap to init paging");
	}

	saveAcpiTables();
	pciInit();

	if(diskDriverInit() < 0)
	{
		panic(PANIC_TYPE_KERNEL, NULL, "Failed to init diskDriver-system");
	}

	print("Disk scan:\n");

	if (diskInit() < 0)
	{
		//kprintf("  [ERROR]: Kernel disk not found\n\n");
		panic(PANIC_TYPE_KERNEL, NULL, "Kernel disk not found");
	}

	int vfslInitVal = vfslInit();
	if (vfslInitVal < 0)
	{
		if (vfslInitVal == -4) // -ENFOUND
			print("No fileSystem found for any disks. Disks are not unusable :/\n");
		else
			panic(PANIC_TYPE_KERNEL, NULL, "Failed to init the virtual filesystem layer");
	}
	
	processInit();
	
	//Prepare first ever process
	int processErrCode = 0;
	struct process* process = createProcess("0:programs/shell.bin", PROCESS_TYPE_USER, &processErrCode);	

	if (process){}
	while(1){}
}

PML4Table getKernelPageTable()
{
	return kernelPageTable;
}

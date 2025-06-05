#include "irqHandler.h"

#include <stdint.h>

#include "idt.h"
#include "print.h"

static int8_t noInterruptHandler(struct trapFrame* frame)
{
	//print("[ERROR] No IRQ handler available\n");
	return 0;
}

static int8_t timerInterrupt(struct trapFrame* frame)
{
	//do nothing for now
	return 0;
}

void initIrqHandler()
{	
	registerHandler(32, timerInterrupt);
	for(uint8_t i = 33; i < 48; i++)
	{
		registerHandler(i, noInterruptHandler);
	}
}

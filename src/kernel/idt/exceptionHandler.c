#include "exceptionHandler.h"

#include <stdint.h>

#include "idt.h"
#include "terminal.h"

static int8_t divError(struct trapFrame* frame)
{
	kprintf("[ERROR] Div by zero/n");
	printTrapFrame(frame);

	while(1){}

	return 0;
}

static int8_t debugError(struct trapFrame* frame)
{
	kprintf("[ERROR] Debug/n");
	printTrapFrame(frame);

	while(1){}
	return 0;
}

static int8_t NoMaskIntError(struct trapFrame* frame)
{
	kprintf("[ERROR] Non-maskable Interrupt/n");
	printTrapFrame(frame);

	while(1){}
	return 0;
}

static int8_t breakError(struct trapFrame* frame)
{
	kprintf("[ERROR] BreakPoint/n");
	printTrapFrame(frame);

	while(1){}
	return 0;
}

static int8_t overflowError(struct trapFrame* frame)
{
	kprintf("[ERROR] Overflow/n");
	printTrapFrame(frame);

	while(1){}
	return 0;
}

static int8_t boundRangeError(struct trapFrame* frame)
{
	kprintf("[ERROR] Bound Range Exceeded/n");
	printTrapFrame(frame);

	while(1){}
	return 0;
}

static int8_t invalidOpcodeError(struct trapFrame* frame)
{
	kprintf("[ERROR] Invalid Opcode/n");
	printTrapFrame(frame);

	while(1){}
	return 0;
}

static int8_t deviceNotAvailableError(struct trapFrame* frame)
{
	kprintf("[ERROR] Device Not Available/n");
	printTrapFrame(frame);

	while(1){}
	return 0;
}

static int8_t doubleFaultError(struct trapFrame* frame)
{
	kprintf("[ERROR] Double Fault/n");
	printTrapFrame(frame);

	while(1){}
	return 0;
}

static int8_t tssError(struct trapFrame* frame)
{
	kprintf("[ERROR] Invalid Tss/n");
	printTrapFrame(frame);

	while(1){}
	return 0;
}

static int8_t segmentPresentError(struct trapFrame* frame)
{
	kprintf("[ERROR] Segment Not Present/n");
	printTrapFrame(frame);

	while(1){}
	return 0;
}

static int8_t stackSegmentError(struct trapFrame* frame)
{
	kprintf("[ERROR] Stack Segment Fault/n");
	printTrapFrame(frame);

	while(1){}
	return 0;
}

static int8_t protectionError(struct trapFrame* frame)
{
	kprintf("[ERROR] General Protection Fault/n");
	printTrapFrame(frame);

	while(1){}
	return 0;
}

static int8_t pageError(struct trapFrame* frame)
{
	kprintf("[ERROR] Page Fault/n");
	printTrapFrame(frame);

	while(1){}
	return 0;
}

static int8_t machineError(struct trapFrame* frame)
{
	kprintf("[ERROR] Machine Check/n");
	printTrapFrame(frame);

	while(1){}
	return 0;
}

static int8_t floatingPointError(struct trapFrame* frame)
{
	kprintf("[ERROR] Floating Point Exception/n");
	printTrapFrame(frame);

	while(1){}
	return 0;
}

static int8_t alignmentError(struct trapFrame* frame)
{
	kprintf("[ERROR] Alignment Check/n");
	printTrapFrame(frame);

	while(1){}
	return 0;
}

static int8_t smidFloatingPointError(struct trapFrame* frame)
{
	kprintf("[ERROR] SMID Floating Point Exception/n");
	printTrapFrame(frame);

	while(1){}
	return 0;
}

static int8_t virtualizationError(struct trapFrame* frame)
{
	kprintf("[ERROR] Virtualization Exception/n");
	printTrapFrame(frame);

	while(1){}
	return 0;
}

static int8_t controlError(struct trapFrame* frame)
{
	kprintf("[ERROR] Control Protection Exception/n");
	printTrapFrame(frame);

	while(1){}
	return 0;
}

static int8_t hypervisorError(struct trapFrame* frame)
{
	kprintf("[ERROR] Hypervisoir Injection Exception/n");
	printTrapFrame(frame);

	while(1){}
	return 0;
}

static int8_t vmmError(struct trapFrame* frame)
{
	kprintf("[ERROR] VMM Communication Exception/n");
	printTrapFrame(frame);

	while(1){}
	return 0;
}

static int8_t securityError(struct trapFrame* frame)
{
	kprintf("[ERROR] Security Exception/n");
	printTrapFrame(frame);

	while(1){}
	return 0;
}

void initExceptionHandler()
{
	registerHandler(0, divError);
	registerHandler(1, debugError);
	registerHandler(2, NoMaskIntError);
	registerHandler(3, breakError);
	registerHandler(4, overflowError);
	registerHandler(5, boundRangeError);
	registerHandler(6, invalidOpcodeError);
	registerHandler(7, deviceNotAvailableError);
	registerHandler(8, doubleFaultError);
	registerHandler(10, tssError);
	registerHandler(11, segmentPresentError);
	registerHandler(12, stackSegmentError);
	registerHandler(13, protectionError);
	registerHandler(14, pageError);
	registerHandler(16, floatingPointError);
	registerHandler(17, alignmentError);
	registerHandler(18, machineError);
	registerHandler(19, smidFloatingPointError);
	registerHandler(20, virtualizationError);
	registerHandler(21, controlError);
	registerHandler(28, hypervisorError);
	registerHandler(29, vmmError);
	registerHandler(30, securityError);
}

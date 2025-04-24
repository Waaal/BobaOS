#include "exceptionHandler.h"

#include <stdint.h>

#include "kernel.h"
#include "idt.h"
#include "terminal.h"

static int8_t divError(struct trapFrame* frame)
{
	panic(PANIC_TYPE_EXCEPTION, frame, "Divide by zero");
	return 0;
}

static int8_t debugError(struct trapFrame* frame)
{
	panic(PANIC_TYPE_EXCEPTION, frame, "Debug");
	return 0;
}

static int8_t NoMaskIntError(struct trapFrame* frame)
{
	panic(PANIC_TYPE_EXCEPTION, frame, "Non-maskable Interrupt");
	return 0;
}

static int8_t breakError(struct trapFrame* frame)
{
	panic(PANIC_TYPE_EXCEPTION, frame, "Breakpoint hit");
	return 0;
}

static int8_t overflowError(struct trapFrame* frame)
{
	panic(PANIC_TYPE_EXCEPTION, frame, "Overflow");
	return 0;
}

static int8_t boundRangeError(struct trapFrame* frame)
{
	panic(PANIC_TYPE_EXCEPTION, frame, "Bound Range Exceeded");
	return 0;
}

static int8_t invalidOpcodeError(struct trapFrame* frame)
{
	panic(PANIC_TYPE_EXCEPTION, frame, "Invalid Opcode");
	return 0;
}

static int8_t deviceNotAvailableError(struct trapFrame* frame)
{
	panic(PANIC_TYPE_EXCEPTION, frame, "Device Not Available");
	return 0;
}

static int8_t doubleFaultError(struct trapFrame* frame)
{
	panic(PANIC_TYPE_EXCEPTION, frame, "Double Fault");
	return 0;
}

static int8_t tssError(struct trapFrame* frame)
{
	panic(PANIC_TYPE_EXCEPTION, frame, "Invalid Tss");
	return 0;
}

static int8_t segmentPresentError(struct trapFrame* frame)
{
	panic(PANIC_TYPE_EXCEPTION, frame, "Segment Not Present");
	return 0;
}

static int8_t stackSegmentError(struct trapFrame* frame)
{
	panic(PANIC_TYPE_EXCEPTION, frame, "Stack Segment Fault");
	return 0;
}

static int8_t protectionError(struct trapFrame* frame)
{
	panic(PANIC_TYPE_EXCEPTION, frame, "General Protection Fault");
	return 0;
}

static int8_t pageError(struct trapFrame* frame)
{
	panic(PANIC_TYPE_EXCEPTION, frame, "Page Fault");
	return 0;
}

static int8_t machineError(struct trapFrame* frame)
{
	panic(PANIC_TYPE_EXCEPTION, frame, "Machine Check");
	return 0;
}

static int8_t floatingPointError(struct trapFrame* frame)
{
	panic(PANIC_TYPE_EXCEPTION, frame, "Floating Point Exception");
	return 0;
}

static int8_t alignmentError(struct trapFrame* frame)
{
	panic(PANIC_TYPE_EXCEPTION, frame, "Alignment Check");
	return 0;
}

static int8_t smidFloatingPointError(struct trapFrame* frame)
{
	panic(PANIC_TYPE_EXCEPTION, frame, "SMID Floating Point Exception");
	return 0;
}

static int8_t virtualizationError(struct trapFrame* frame)
{
	panic(PANIC_TYPE_EXCEPTION, frame, "Virtualization Exception");
	return 0;
}

static int8_t controlError(struct trapFrame* frame)
{
	panic(PANIC_TYPE_EXCEPTION, frame, "Control Protection Exception");
	return 0;
}

static int8_t hypervisorError(struct trapFrame* frame)
{
	panic(PANIC_TYPE_EXCEPTION, frame, "Hypervisoir Injection Exception");
	return 0;
}

static int8_t vmmError(struct trapFrame* frame)
{
	panic(PANIC_TYPE_EXCEPTION, frame, "VMM Communication Exception");
	return 0;
}

static int8_t securityError(struct trapFrame* frame)
{
	panic(PANIC_TYPE_EXCEPTION, frame, "Security Exception");
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

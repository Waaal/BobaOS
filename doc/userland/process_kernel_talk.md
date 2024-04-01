# Talk with the kernal as a Process
Userland and kernel communication is done via a interrupt. The Userprocess is calling a interrupt, a kernel interrupt routine performs the interrupt and returns the result back to the process (the result is normaly in the eax register).

**Note: When we call the kernel with the int instruction the processor pushes the same information to return to userland as we did in the [Userland](Userland.md) doc. So it pushes the Data segment, stack address etc in the right order so when we are finished with the kernel routine we can simply leave kernel land and return back to userland with a iret instruction**

## How to hand a interrupt
To handle a interrupt we need to make a entry in the IDT with the vector we want the userland to use for kernel interrupts.
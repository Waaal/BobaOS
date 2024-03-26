# Userland
Userland is a term used to describe when the processor is in a limited, privileged state. Operating system processes run in user land. (Kernel land is the state thate the kernel runs in.)

Userland is when the processor is in ring3. Userland is safer because if something goes wrong the kernel is able to intervene. When the processor is in user land it is unable to execute privileges instructions such as chaning the GDT, IDT, paging or direct hardware access with in and out.

## Getting to user land for the first time
- Setup user code and data segments in GDT
- Setup a TSS
- Get to user land with iret

### Setupt userland and data segments
To be able to switch to userland we need to have a user data and code segment in the GDT.

### Setupt TSS
Explenation of the TSS is [here](../STCQ/TSS.md)

### Get to userland with iret
To get to userland we need to pretend we returned from a interrupt and want to get back to ring3. Because the CPU dont have a direct command to change priviledge level.

- We need to change the data segments (ds,fs,es,gs) to User Data segment (dont need to change the ss because the iret instruction does this on its own)
- Push user data segment to stack
- Push userprogram stack pointer to stack
- Push current flags to stack and bitwise OR the bit that re-enables interrupts (0x200)
- Push user code segment to stack
- Push the program counter (PC) also called instruction pointer (IP) (the address of the next line of code or entry point) 
- Call iret (iretd for 32 bit | iretq for 64 bit)

So the iret instruction expects the following structure on the stack:
```
0x100
		| user data segment |
		| stack pointer |
		| current flags |
		| user code segment |
		| IP register |
		...
0x0
```
This is a basic explenation on how to get to ring3. If this is not the fist time we jump back to this task in ring3 then we also need to restore all of the registers of this programm before doing the iret instruction

## Switching between userland and kernelland
When we call a function from the userland or a interrupt happens, the CPU automatically pushes all registers to the stack. So if we are in kernelland and want to go back to userland with the iret instruction we need to restore all of these general purpose registers and then return back to userland with iret.

## Switching between processes
With the help of the PIC Timer interrupt we can switch a process every time the timer interrupt is fired.


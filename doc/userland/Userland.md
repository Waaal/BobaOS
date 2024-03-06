# Userland
Userland is a term used to describe when the processor is in a limited, privileged state. Operating system processes run in user land. (Kernel land is the state thate the kernel runs in.)

Userland is when the processor is in ring3. Userland is safer because if something goes wrong the kernel is able to intervene. When the processor is in user land it is unable to execute privileges instructions such as chaning the GDT, IDT, paging or direct hardware access with in and out.

## Getting to user land for the first time
- Setup user code and data segments
- Setup a TSS
- Get to user land with iret

### Setupt user cand and data segments
To be able to swithc to userland we need to have a user data and code segment in the GDT. We alos need a TSS entry in the GDT.

### Setupt TSS
Explenation of the TSS is [here](../STCQ/TSS.md)

### Get to userland with iret
To get to userland we need to pretend we returned from a interrupt and want to get back to ring3. Because the CPU dont have a direct command to change priviledge level.

- We need to populate our data segment register with the code and data segment for the user entry in the GDT
- Save stack pointer into eax
- Push user data segment to stack
- Push stack pointer in eax to stack
- Push current flags to stack and bitwise OR the bit that re-enables interrupts
- Push user code segment to stack
- Push address of the function we want to run in userland
- Call iret

The iret instruction expects the following structure on the stack:
```
0x100
		| user data segment |
		| stack pointer |
		| current flags |
		| user code segment |
		| address of function | <-- [Stack pointer]
		...
0x0
```

## Switching between userland and kernelland
When we call a function from the userland or a interrupt happens, the CPU automatically pushes all registers to the stack. So if we are in kernelland and want bakc to userland with the iret instruction we need to restore all of these general purpose registers and the returen back to userland with iret.

## Switching between processes
With the help of the PIC Timer interrupt we can switch a process every time the timer interrupt is fired.


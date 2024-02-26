# Userland
Userland is a term used to describe when the processor is in a limited, privileged state. Operating system processes run in user land. (Kernel land is the state thate the kernel runs in.)

Userland is when the processor is in ring3. Userland is safer because if something goes wrong the kernel is able to intervene. When the processor is in user land it is unable to execute privileges instructions such as chaning the GDT, IDT, paging or direct hardware access with in and out.

## Getting to user land
- Setup user code and data segments
- Setup a TSS
- Pretent we are returning from an interrupt and execute iret instruction

### Setupt user cand and data segments
To be able to swithc to userland we need to have a user data and code segment in the GDT. We alos need a TSS entry in the GDT.

### Setupt TSS
Explenation of the TSS is [here](../STCQ/TSS.md)

### return to userland with iret
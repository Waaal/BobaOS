# Task State Segment
*Note: This is still a early article. The information may be wrong and/or incomplete.*

A TSS is a struct that lives in memory. It holds information about a Task.

## TSS in protected mode (32bit)
In protected mode TSS is responsible for holding information such as the kernel stack pointer and segment.
It also holds the General purpose registers, Segment Selectors, Instruction pointer, EFLAGS register and the Cr3. It holds a programs state

So if there is a interrupt in the user land the TSS will be populatet with information. With this the CPU can repopulate its registers after we returned from the interrupt and it can execute the userland programm where it left of.

The processor is populating this structur automatically in a interrupt event.

Important for us are the Kernel stack pointer (ESP0) and the kernel stack segment (SS0). 

| 4 Bytes | Offset |
| ------ | ------ | 
| SSP | 0x68 |
| IOPB *(2byte (high))* | 0x64 |
| LDTR *(2byte)* | 0x60 |
| GS *(2byte)* | 0x5C |
| FS *(2byte)* | 0x58 |
| DS *(2byte)* | 0x54 |
| SS *(2byte)* | 0x50 |
| CS *(2byte)* | 0x4C |
| ES *(2byte)* | 0x48 |
| EDI | 0x44 |
| ESI | 0x40 |
| EBP | 0x3C |
| ESP | 0x38 |
| EBX | 0x34 |
| EDX | 0x30 |
| ECX | 0x2C |
| EAX | 0x28 |
| EFLAGS | 0x24 |
| EIP | 0x20 |
| CR3 | 0x1C |
| SS2 *(2byte)* | 0x18 |
| ESP2 | 0x14 |
| SS1 *(2byte)* | 0x10 |
| ESP1 | 0x0C |
| SS0 *(2byte)* | 0x08 |
| ESP0 | 0x04 |
| LINK *(2byte)* | 0x00 |

**LINK:** The segment selector for the TSS of the previous task

**SS0, SS1, SS2:** Segment selectors to load stack for priviledge level change from ring 0 - 2

**ESP0, ESP1, ESP2:** Stack pointer load stack when priviledge level change from ring 0 - 2

**IOPB:** I/O Map Base address field for I/O port permission map

**SSP:** Shadow stack pointer

## TSS in long mode (64bit)
In long mode does not store a tasks execution state. It stores the kernel stack pointer and if used the Interrupt stack table.

## TSS more info
The TSS is also needed for software multitasking. Because each CPU core has its own TSS in a multitasking scenario.

### TSS in a GDT
The TSS needs to have a entry in the gdt. The TSS entry is special, because the base and limit needs to be set to the start and size of the TSS.

Base = TSS start address.
Limit = TSS Length.

The Access Bytes look like this: **1110 1001** 
Special about this is that the bit 4 (Descriptor Type) is set to 0 which means this is a system segment and not a code or data segment.

## Load the TSS
The tss needs to be loaded with the ltr instruction. The ltr instruction taks as argument the TSS GDT entry.
``` assembly
    mov ax, 0x28 	;0x28 = 00000000 00101000 = RPL = 0; TL = 0; Selected Index = 5 (At which index is the TSS entry in the GDT);
    ltr ax
```

## Implementation  
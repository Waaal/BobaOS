# Global Descriptor Table
GDT is a struct which lives in memory and is used by the CPU to protect memory.
A GDT can have up to 8000 entries.

## Explenation
When the processor accesses some memory, the value in the segment register is an index in the GDT. Instead in real mode, where the value in the segment register just holds a offset and is shiftet to the left 4 bits.

A entry in the GDT holds the **base address** of a segment, **segment limit** and the **segment attributes**. 
The segment attributes hold a priviledge level, to see if we have access to this memory segment. So with a GDT we can protect memory with different privileges levels from the user and user applications. 

Nowerdays the memory protection is done with paging. So in a 32 bit operating system the GDT just holds some default values and the base address and segment limit spans the entire memory address space.

In long mode pagin is required and the base address and sagment limit are ignored.

Normaly on modern operating systems there are 2 entrys in the GDT per ring. A data and a code entry. So the kernel (ring 0) has a code and a data entry and the user (ring 3) has a code and a data entry.

The CS segment points to the Code segment in the GDT. 
The other registers (SS, DS, FS, ES, GS) points to the Data segment in the GDT.

*Note: There is also a LDT. A Local Descriptor Table. It is build exactly like a GDT with the difference, that every task/thread can have its own LDT. So a LDT can exists for every task/thread while a GDT exists only one times.*

### Data structure in a segment register
The Data in a segment register is 16 bit long both in protected and long mode and looks like following:
```
15                      3  2  1  0
|     Selected Index    |  TI | RPL |
```
**Selected Index:** Points to the Entry in the GDT.

**TL:** Can be 0 or 1. 0 Means check in GDT. 1 Means check in LDT.

**RPL:** Requested priviledge level. So if we are on ring3 then the RPL would be 3. If we are on ring 0 the RPL is 0.

This Data struct is alled a **Segment Selector**.

### Data structure of a GDT entry.
A GDT entry is 64 bit long in protected mode and 128 bit long in long mode.
(The data structure for a TSS entry looks different.[See here for a TSS entry](Tss.md))
```
<---------LONG MODE---------> 
127            96 95      64 63     56 55   52 51   48 47           40 39       32 31           16 15           0
|    Reserved    |   Base   |   Base  | Flags | Limit | Access Bytes  |    Base   |      Base     |     Limit   |
```
**Base:** A 32 Bit (long mode 64 bit) value containing the address, where the segment begins.

**Limit:** A 20 bit value containing the maximum address unit of this segment. Either in byte units or in 4KiB pages. (Flags bit 3)

*Note: In long mode Base and Limit are ignored. Each entry covers the entire linear address space regardless of what they are set to. There is one case where base and limit are not ignored in long mode and that is in the TSS entry.*

---
**Access Bytes (For code/data segment):**
```
 7   6   5   4   3   2   1   0
|P |  DPL |  S|  E|  DC| RW| A|
```
**P:** Present bit. Allows an entry to refer to a valid segment. Must be 1 for any valid segment

**DPL:** Descriptor privilege level. Contains the privileges level of the segment.

**S:** Descriptor type. 0 = Descriptor defines a System segment. 1 = Descriptor defines a code or data segment.

**E:** Executable bit. 0 = Descriptor defines a Data sagment. 1 = Descriptor defines a code segment, which is executable.

**DC:** Direction bit/Conforming bit
- For Data (Direction bit): 0 = segment gorws up. 1 = segment grows down (Offset has to be grather than the limit).
- For Code (Conforming bit): 0 = Code can only be executet from the ring, which is set in DPL 1 = Code can be executet with equal or lower priviledges rights.

**RW:** Readable/Writable bit.
- For Data (Writable): 0 = Write is denied. 1 = Write is allowed. Read is always allowed
- For Code (Readable): 0 = Read is not allowed. 1 = Read is allowed. Write is never allowed

**A:** Access bit. Best practice is to leave this at 0. CPU will set it, when the segment is accessed.

---
**Access Bytes (For system segment bit 4 is 0)**

The Access bytes in a system segment are stored differently, than the normal access bytes. (A system segment is used for a TSS or LDT entry.)
```
 7   6   5   4   3   2   1   0
|P |  DPL |  S|      Type    |
```
**Type:** Type of system segment.
- Types in 32 bit protected mode:
    - 0x1: 16-bit TSS (available)
    - 0x2: LDT
    - 0x3: 16-bit TSS (busy)
    - 0x9: 32-bit TSS (available)
    - 0xB: 32-bit TSS (busy)
- Types in Long mode:
    - 0x2 LDT
    - 0x9 64-bit TSS (available)
    - 0xB 64-bit TSS (busy)

---
**Flags:**
```
 3  2   1   0
|G |DB| L|  Reserved
 ```
**G:** Granularity flag. The size the Limit value is scaled by. 0 = The Limit is 1 byte blocks. 1 = The Limit is 4 kiB blocks.

**DB:** Size flag. 0 = describtor defines a 16 bit protected mode segment. 1 = descriptor defines a 32 bit portected mode.

**L:** Long mode code flag. 1 = defines a 64 bit code segment. If DB is 1 then L should always be 0.

### Load a GDT
To load a Global Descriptor Table we need a pointer to this table.
The pointer needs to have the following structure:

**32-Bit:**
```
31          16 15       0
|   Offset    |  Size   |
```

**64-Bit:**
```
63          16 15       0
|   Offset    |  Size   |
```

**Size:** Size of the IDT - 1;

**Offset:** Staring address of the IDT.

Load the GDT with the lgdt instruction. The argument of the lgdt is 
``` assembly
lgdt [gdt_ptr]
```

## Implementation
The Implementation normaly has to be done in assembly code, because a 32 bit GDT is required to jump to protected mode and a 64 bit GDT is required to jump tol ong mode.

### Assembly
This example is for **protected mode, 32 bit** GDT.
``` assembly
gdt_start:
gdt_null: dq 0 				; NULL Entry in gdt. One entry is 8 byte in protected mode

gdt_code:		 		; Code Segment (CS should point to this). Just some default values, because we are going to use paging.
	dw 0xffff			; Segment Limit (0-15 bits) Set to the full address range, because we use paging.
	dw 0				; Base (0 - 15) Set to 0.
	db 0				; Base (16 - 23) Set to 0.
	db 0x9a				; Access bites P = 1, DPL = 0 (ring 0 entry), S = 1 (Code/Data), E = 1 (Code), DC = 0 (Code can only be
					; executed from ring set in DPL), RW = 1 (Read is allowed), A = 0  
	db 11001111b			; Flags to 1100 and Limit to 1111. Flags: G = 1, DB = 1 (protected mode) L = 0 (protected mode)
	db 0				; Base 24-31 Set to 0.

gdt_data:				; Data Segment, stack segment (DS,SS,ES,FS and GS should point to this). Also just some default values
	dw 0xffff			; Segment Limit (0-15 bits) Set to the full address range, because we use paging.
	dw 0				; Base (0 - 15) Set to 0
	db 0				; Base (16 - 23) Set to 0
	db 0x92				; Access bites P = 1, DPL = 0 (ring 0 entry), S = 1 (Code/Data), E = 0 (Data), DC = 0 (Grow down)
					; RW = 1 (Read and write is allowed), A = 0  
	db 11001111b			; Flags to 1100 and Limit to 1111. Flags: G = 1, DB = 1 (protected mode) L = 0 (protected mode)
	db 0				; Base 24-31 Set to 0
gdt_len: equ $ - gdt_start

gdt_ptr:				; gdt pointer to use in the lgdt instruction.
	dw gdt_len			; 2 Byte
	dd gdt_start			; 4 Byte (protected mode)
```
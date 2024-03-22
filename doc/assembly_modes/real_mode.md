# Real Mode
Real mode (Real address mode) is the mode the CPU is in after booting.

In real mode the CPU and all of our registers are in 16 bit mode. 

We can use 1MB of RAM and have no memory protection (pagind, GDT).

The use of the segment registers are to access memory larger than 16bit.

## Usable registers
We have access to the following registers:

**General registers:**
- AX: [AH | AL]
- BX: [BH | BL]
- CX: [CH | CL]
- DX: [DH | DL]
- SI: Source Index
- DI: Destination Index

**Stack registers:**
- SS: Stack segment (Also a segment register)
- SP: Stack pointer

**Segment registers:**
- CS: Code segment
- DS: Data segment
- SS: Stack segment
- ES: Extra segment
- FS: General purpose segment
- GS: General purpose segment

## Memory Access (Segmentation)
Because our registers are only 16 bit we could only access memory in a 16 bit range.
To access memory larger than 16 bit we use the segment registers.

Because in Real mode in a segment register the number is shiftet 4 bits to the left.
```assembly
	; I want to access address 0xC8000. 0xC8000 is too large for 16 bit
	mov ax, 0xC800
	mov es, ax				; Move pointer to 0xC800 into Segmentation register es
	mov ax, byte[es:0x00]			; Access variable 0xC8000 because everything in the Segmentation registers is shifted 4 byte.
						; es:0x00 = 0xC800:0x00 = 0xC800 * 16 + 0x00 = 0xC8000
```

## BIOS Functions
**Only** in real mode we have access to BIOS functions.

Use Ralf Browns interrupt list, to look at all the BIOS functions. (BEST resource yes)
https://www.ctyme.com/intr/int.htm
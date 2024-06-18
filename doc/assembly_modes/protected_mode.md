# Protected Mode
Protected mode is 32 bit. In this mode we have the full 32 bit addressing mode. So we can use max 4GB of ram.
We dont have access to BIOS functions anymore. 

We now have access to important structs like the GDT, IDT and paging.


The use of the segment registers is to hold a segment selector struct with a entry in the GDT.


A stack entry is now 32 bit and pointer are 32 bit.

## Usable registers
We have access to the following registers:

**General registers:**
- EAX: [AX]
- EBX: [BX]
- ECX: [CX]
- EDX: [DX]
- ESI: Source Index
- EDI: Destination Index

**Stack registers:**
- SS: Stack segment (Also a segment register)
- ESP: Stack pointer

**Segment registers:**
- CS: Code segment
- DS: Data segment
- SS: Stack segment
- ES: Extra segment
- FS: General purpose segment
- GS: General purpose segment

## Jump to protected mode
To jump from real mode to protected mode we need to have a valid GDT.
After we load the GDT we can ebable bit 1 in the controll register to enable protected mode.


After that we need to set all the segment registers need to hold a valid segment selector.


``` assembly
; CODE_SEG equ gdt_code - gdt_start	= Get offset in GDT table for code (0x8 = 00001000 = Selected Indedx:1, TI:0, RPL:0)
; DATA_SEG equ gdt_data - gdt_start	= Get offset in GDT table for data (16 =00010000 = Selected Index:2, TI:0, RPL:0)

	cli					; Disable interrupts
	lgdt [cs:gdt_ptr]

	mov eax, cr0
	or eax, 0x1
	mov cr0, eax		; Enable bit 1 in controll register 0 = protected mode
	
	jmp CODE_SEG:load32

[bits 32]
load32:
	
	mov eax, DATA_SEG
	mov ds, eax
	mov es, eax
	mov fs, eax
	mov gs, eax
	mov ss, eax
	
```
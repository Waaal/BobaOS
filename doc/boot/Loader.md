# Loader
*This document is a basic Explenation what the loader from the BobaOS does.*
## Job of the loader
The loader is loaded and called from the bootloader.
Its basic job is to prepare everything for the kernel and load the kerner.
In detail it does the following:
- Check CPU features (Long mode, etc)
- Get Memory map info
- Test for A20 Line
- Set the Video mode
- Set up and jump to protected mode
- Load the kernel
- Set up and jump to long mode
- Move kernel and Jump to kernel

## Check CPU features
Check for extended function. Extendet function is needed, to check for long mode and other CPU features.
```assembly
mov eax, 0x8000000
cpuid
cmp eax, 0x80000001
```
If eax is less than 0x80000001 that means that exteded function is not available.
If extended function is available check for long mode.
```assembly
mov eax, 0x80000001
cpuid 
est edx, 1 << 29      ; Check for bit 29, which is long mde
```
If Bit 29 is zero, that means there is no long mode.
## Get Memory Map
**[Not Implemented]**
## Test A20
A20 Line needs to be on. Because with A20 line, we can access addresses, bigger than 20 bit.
If A20 Line is off addresses bigger than 20 bit gets truncated:
```
0x1080C8    -   1 0000 1000 0000 1100 1000
       truncated to
0x0080C8    -   0 0000 1000 0000 1100 1000
```
We check if A20 Line is on, by writing a value to 0x0080C8. 
Then we write a **different** value to 0x1080C8.

If we now look at 0x0080C8 and check if the value is the same value we wrote in 0x1080C8, that means that A20 Line is off, because 0x1080C8 got truncatet and overwrote 0x0080C8.

If A20 line is off, we stop the boot process.
```assembly
; Note - please dont write in random memory addresses. This is just a example.
mov ax, 0xFFFF
mov es, ax
mov word[0x80C8], 0xa200
mov word[es:0x80D8], 0xb200     ; 0xFFFF * 16 + 0x80D8 = 0x1080C8
cmp word[0x80C8], 0xb200
je A20IsOffErr
```

## Set Video mode 
We set video mode to text.
After we made the jump to protect mode, we cannot write to the screen anymore through BIOS functions, because they are not available in protected mode. 

In Video mode we have 20 rows and 80 columns we cann fill with characters. One Character is 2 byte. First byte is ascii char value, the second byte is char attribute.
The first 4 bit of the attribute byte are forground color. The second 4 bit are background color.

Video memory starts at 0xb8000.
We write to the screen, by just writing the bytes in video memory.
Enable video mode:
```assembly
mov ax, 3       ; 3 means text mode
int 0x10        ; Call Video service to change video mode.
```
Write with video mode:
```assembly
    mov si, Message
    mov cx, MessageLen
    mov ax, 0xb800
    mov es, ax
    xor di, di
PrintMessage:
    mov al, [si]
    mov[es:di], al          ; Write character char in first byte
    mov byte[es:di+1] 0xD   ; Write char attribute in second byte
    
    add di, 2               ; Move 2 bytes forward in video memory
    add si, 1               ; Move 1 byte forward to next char in Message
    loop PrintMessage
```
## Load Kernel
Next step is to load the kernel. The memory reserved for the kernel is 500kb.
## Prepare for Protected mode
We need to set up a few thing before we leave real mode and enter protected mode
- GDT (Global Descriptor Table)
- IDT (Interrupt Descriptor Table)

### Global Descriptor Table
GDT is a struct which lives in memory and is used by the CPU to protect memory.
A Table entry is 8 byte long. The first 8 bytes need to be empty in the GDT.
A GDT can have up to 8000 entries.

When the processor accesses some memory, the value in the segment register is an index in the GDT. Instead in real mode, where the value in the segment register just holds a offset and is shiftet to the left 4 bits.

A entry in the GDT holds the **base address** of a segment, **segment limit** and the **segment attributes**. 
The segment attributes hold a priviledge level, to see if we have access to this memory segment. So with a GDT we can protect memory with different privileges levels from the user and user applications.

*Note: There is also a LDT. A Local Descriptor Table. It is build exactly like a GDT with the difference, that every task/thread can have its own LDT. So a LDT can exists for every task/thread while a GDT exists only one times.*

##### How to check priviledges?
To find out at which priviledge level we are currently running (0,1,2,3) we need to check the CPL (Current Privilege Level). The CPL is stored in the lower 2 bits of a segment register.

We remeber the segment registers in real mode was used to access memory higher than 2 bytes. But in portected and long mode the segment registers are used to store our current privliedge level and the entry in the GDT.

So if the lower 2 bits of a Segment Register are 0, we are running in ring 0.

Normally the kernel and userspace each has 2 entrys in the GDP. One for code and one for data.

##### Example Data in a Segment Register:
```
15                      3  2  1  0
|     Selected Index    |  TI | RPL |
```
**Selected Index:** Points to the Entry in the GDT.

**TL:** Can be 0 or 1. 0 Means check in GDT. 1 Means check in LDT.

**RPL:** Requested priviledge level. The priviledges we have.


This Data struct is alled a **Segment Selector**.

If the CS register is used, it normally points to the code entry in the gdt. For all the other registers (SS, DS, FS, ES, GS) the data entry is used.

##### Data in a Table Entry (protected mode):
The Data Structure of a DPL is calles a System Segment Descriptor and is structured like this:
```
63      56 55   52 51   48 47           40 39       32 31           16 15           0
|   Base  | Flags | Limit | Access Bytes  |    Base   |      Base     |     Limit   |
```
**Base:** A 32 Bit value containing the address, where the segment begins.

**Limit:** A 20 bit value containing the maximum address unit of this segment. Either in byte units or in 4KiB pages. (Flags bit 3)

*Note: In long mode Base and Limit are ignored. Each entry covers the entire linear address space regardless of what they are set to*. There is one case base and limit are not ignored in long mode. When we set up a TSS and add a TSS entry in the GDT.

---
**Access Bytes:**
```
 7   6   5   4   3   2   1   0
|P |  DPL |  S|  E|  DC| RW| A|
```
**P:** Present bit. Allows an entry to refer to a valid segment. Must be 1 for any valid segment

**DPL:** Descriptor privilege level. Contains the privileges level of the segment.

**S:** Descriptor type. 0 = Descriptor defines a System segment. 1 = Descriptor defines a code or data segment.

**E:** Executable bit. = 0 Descriptor defines a Data sagment. 1 = Descriptor defines a code segment, which is executable.

**DC:** Direction bit/Conforming bit
- For Data (Direction bit): 0 = segment gorws up. 1 = segment grows down (Offset has to be grather than the limit).
- For Code (Conforming bit): 0 = Code can only be executet from the ring, which is set in DPL 1 = Code can be executet with equal or lower priviledges rights.

**RW:** Readable/Writable bit.
- For Data (Writable): 0 = Write is denied. 1 = Write is allowed. Read is always allowed
- For Code (Readable): 0 = Read is not allowed. 1 = Read is allowed. Write is never allowed

**A:** Access bit. Best practice is to leave this at 0. CPU will set it, when the segment is accessed.

---
**Access Bytes (For system segment)**

The Access bytes in a system segment are stored differently, than the normal access bytes.
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


##### Data in a Table Entry (TSS):
The System Segment Descriptor is for a TSS slightly different.
```
127    96 95        64 63     56 55   52 51   48 47           40 39       32 31           16 15           0
 | ///// |    Base    |   Base  | Flags | Limit | Access Bytes  |    Base   |      Base     |     Limit   |
```  


### Interrupt Descriptor Table
IDT is a struct which lives in memory and is used by the CPU to find interrupt and exception handlers.
A interrupt can be a signal send from a device, or a Exception generatet by the CPU to alert the Kernel. If the CPU accepts a interrupt, it will stop the current task and process the interrupt. A IDT can have up to 256 entries.


There are 3 different types of interrupts:
- Exception: Generated by the CPU to alert kernel.
- Interrupt Request (IRQ) (Hardware interrupt): Generatet by a external chipset.
- Software Interrupts: Signaled by software running on CPU. Mostly done by System Calls with INT instruction.  


For hardware interrupts please see [HardwareInterrupts](HardwareInterrupts.md)


Different interrupts and exceptions have different Numbers which are called interrupt vectors. With a IDT a CPU can look up a interrupt number and find the handler, for this specific event. For example the interrupt from a keyboard has the number 1. So the CPU searches in the IDT for a handler, which can process interrupts with the interrupt number 1 (a keyboard handler). Vectore 0 - 31 are pre defined by the CPU. 
Numbers from 32 - 255 can be used by the operating system.

A entry in an IDT holds in which **segment** the interrupt service routine is locatet and the **offset** to the routine.

##### Data in a Table Entry:
On entry is 18 bytes (128 bits) long and is structured like this:
```
 127         96 95          64 63      48 47           40 39  34 34    32 31           16 15           0
  |   //////   |    Offset    |  Offset  |  Attributes   | //// |   IST  |    Selector   |    Offset   |
```


**Offset:** 64 bit value split in 3 parts. Is the address of the entry point of the Interrupt service routine.

**Selector:** A Segment Selector. Must point to a valid code segment.

**IST:** Offset in the interrupt stack table. If bits are all zero IST is not beeing used.

**Attributes:** 

```
 7   6   5   4   3   2   1   0
|P |  DPL |  0|   Gate Type   |
```

**P**: Present bit. Must be 1 for this entry to be valid

**DPL:** A priviledge level, which are allowed to access this interrupt via INT instruction. Hardware interrupts ignore this.

**Gate Type:** Describe the type of gate, this interrupt descriptor represents. In long mode there are 2 values:
- 0xE (0b1110) = 64 bit Interrupt Gate
- 0xb (0b1111) = 64 bit Trap Gate

## Jump to Protected mode
In order to jump to protected mode, we need to set up and fill a GDP with data.
The most importand fields in the GDP, are the on for the Kernel. Because the Kernel (and the loader) needs to have rights after the jump.
After the jump, the loader ist still active and uses the rights of the Kernel, to do his work.
``` assembly
Gdt32:
    dq 0        ; Fist 8 bytes are emtpy
Code32:         ; Second entry for the Code of the kernel
    dw 0xffff   ; Set Limit to maximum
    dw 0        ; Set Base address to 0
    db 0        
    db 0x9a     ; Access bytes to 1001 1010
    db 0xcf     ; Set Flags to 1100 and Limit to 1111
    db 0
Data32:         ; Third Entry for the Data of the Kernel
    dw 0xfff    ; Limit to maximum
    dw 0        ; Base to 0
    db 0    
    db 0x92     ; Access bytes to 1001 0010
    db 0xcf     ; Flags to 1100 and Limit to 1111
    db 0

Gdt32Len: equ $ - Gdt32

Gdt32Ptr: dw Gdt32Len       ; Pointer struct to load a GDT (load IDT has same potiner struct)
          dd Gdt32          ; First 4 bytes are length of the table. Second 2 bytes are actuall address of the table
```

**Code access explenation:**

We set P bit to 1 which means this sector is valid.

We set DPL bits to 0 which means to access this sector we need ring0 access rights 

We set S bit to 1 which means this is a code or data segment

We set E bit to 1 which means this is a Code segment

We set DC bit to 0 which means code can only be executet from the rights, we wrote in DPL.

We set RW bit to 1 which means Read is allowed

We set A bit to 0. Because it is set by the CPU

**Code Flags explenation:**

We set G to 1 which means the limit is 4KiB block

We set DB to 1 which means 32 bit protected mode

Wet set L to 0 which means no long mode

And the last bit is reserved


**Data Access bit explenation:**

We set P bit to 1 which means this sector is valid.

We set DPL bits to 0 which means to access this sector we ned ring0 access rights 

We set S bit to 1 which means this is a code or data segment

We set E bit to 0 which means this is a Data segment

We set DC bit to 0 which means data in the segment grows up.

We set RW bit to 1 which means Write is allowed

We set A bit to 0. Because it is set by the CPU


Now we have defined our struct. We give it to the cpu before we leave protected mode.
For this we make a GdtPtr struct
``` assembly
GdtPtr32:
    dw GdtLen-1         ; first 2 bytes is the length of our gdt table -1
    dd Gdt              ; Last 4 bytes is the address of the gdt. (in 64 bit mode this address is 8 bytes long)
```
``` assembly
    cli                 ; Clear interrupt flag and disable all interupts. (We enable them in 64 bit mode)
    lgdt [GdtPtr32]     ; Load our GDT
    lidt [IdtPtr32]     ; Load IDT Table. (Empty pointer struct. Same Structured as GdtPtr32 bit with all 0s)

    mov eax, cr0        ; Cr0 is a CPU controll register. We load the cr0 controll register and set it to 1. Enabling protected mode 
    or eax, 1
    mov cr0, eax

    ; Now we do the jump. 8 is here the offset in our GDT table. We remember, our code table Entry was at index 1.
    ; With this jump we load the cs register with the needed data.
    ; 8 = 00001000 = Selected index is 1; TI = 0; RPL = 0;
    ; (Look at "Example Data in a Segment Register" for more info)
    jmp 8:PMEntry
[Bits32]
PMEntry:
    ; We load 10 in all the other segment registers for the Data. 0x10 = 16 = 00010000 = Selected index is 2; TI = 0; RPL = 0
    mov ax, 0x10 
    mov es, ax
    mov ds, ax
    mov ss, ax
```

We have now completet the jump.
## Prepare for long mode
Long mode comes in 2 sub modes. The actual 64 bit mode and the 32 bit compability mode. We use the 64 bit mode.
To prepare for long mode, we need to set up a couple of things.
- Enable Physical address extensions
- Set up a paging struct and enable Paging
- Set up a 64bit GDT
- Enable long mode and jump to it


### Physical address extension
When physical address extension (PAE) is on, each entry in the page table goes up to 64 bit instead of the normal 32 bit. This allows us to locate above the 4GB boundary.
``` assembly
    mov eax, cr4            ; PAE bit is in controll register 4
    or eax (1>>5)           ; PEA is bit number 5
    mov cr4, eax            ; Write it back in controll register
```
### Paging

Please see [Paging](../paging.md)

Enable Paging:
``` assembly
    mov eax, 0x70000        ; Write paging struct in cr3
    mov cr3, eax

    mov eax, cr0            ; Enable paging in controll register 0
    or eax, (1>>31)
    mov cr0, eax
```
### Set up GDT
We set up another GDT for the long mode. As mentiond the GDT in long mode is slightly different.
The field Limit and Base are completly ignored.
Also ignored in some Attributes are:


Ignored in Access bytes:
- RW
- A

Ingored in Flags:
- G
- Reserved

``` assembly
Gdt64:
    dq 0                    ; First entry is empty
    dq 0x0020980000000000   ; Access bytes: P = 1; S = 1; E = 1; Flags: L = 1;
                            ; Dont need a data entry just yet  
```
``` assembly
    lgdt Gdt64Ptr           ; Note pointer is 8 bytes long
```
### Enable and Jump to long mode
``` assembly
    mov ecx, 0xc0000080
    rdmsr                   ; Access the model state register
    or eax, (1<<8)          ; Set bit 8 to 1
    wrmsr                   ; Write it back to the MSR

    jmp 8:LMEntry

[BITS 64]                   ; Dont forget bits directive
LMEntry:
    mpv rsp, 0x7c00
```
## Prepare for long mode
Last step is to rellocate the kernel from memory address 0x1000 to 0x200000
``` assembly
    cld
     mov rdi, 0x200000      ; Destionation address
     mov rsi, 0x10000       ; Source address
     mov rcx, 51200/8       ; Counter. We move 8 bytes per iterration. Kernal has 100 Sektors. So 512*100.
     rep movesq             ; Repeat until rcx is 0. move 8 bytes from source to destination

     jmp 0x200000           ; Jump to kernel. (We are running already in ring0)
```
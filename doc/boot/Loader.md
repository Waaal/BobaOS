# Loader
*This document is a basic Explenation what the loader from the BobaOS does.*
## Job of the loader
The loader is loaded and called from the bootloader.
Its basic job is to prepare everything for the kernel and load the kerner.
In detail it does the following:
- Check CPU features (Long mode, etc)
- Get Memory map info
- Test for A20 Line
- Set the Vido mode
- Set up and jump to protected mode
- Load the kernel

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
A Table entry (DPL = Descriptior Table Entry) is 8 byte long. The first 8 bytes need to be empty in the GDT.
A GDT can have up to 8000 entries.

When the processor accesses some memory, the value in the segment register is an index in the GDT. Instead in real mode, where the value in the segment register just holds a offset and is shiftet to the left 4 bits.

A entry in the GDT (a DPL) holds the **base address** of a segment, **segment limit** and the **segment attributes**. 
The segment attributes hold a priviledge level, to see if we have access to this memory segment. So with a GDT we can protect memory with different privileges levels from the user and user applications.

*Note: There is also a LDT. A Local Descriptor Table. It is build exactly like a GDT with the difference, that every task/thread can have its own LDT. So a LDT can exists for every task/thread while a GDT exists only one times.*

##### How to check priviledges?
To find out at which priviledge level we are currently running (0,1,2,3) we need to check the CPL (Current Privilege Level). The CPL is stored in the lower 2 bits of cs and ss register.
So if the lower 2 bits of ss and cs register are 0, we are running in ring0.

If we try to access memory, our RPL (Requestet Privilege Level, which is stored in the selectors) is compared against the DPL, which can be found in the GDT for the current sector, and if this tests fails, we dont have access and a exception is generated.

##### Example Data in a Segment Selector:

```
15                      3  2  1  0
|     Selected Index    |  TI | RPL |
```
**Selected Index:** Points to the Entry in the GDT
**TL:** Can be 0 or 1. 0 Means check in GDT. 1 Means check in LDT.
**RPL:** Requested priviledge level. The priviledges we have.

**[NOT FINSIHED]**

### Interrupt Descriptor Table
IDT is a struct which lives in memory and is used by the CPU to find interrupt handlers.
A interrupt is a signal, send from a device to the CPU. If the CPU accepts a interrupt, it will stop the current task and process the interrupt. A IDT can have up to 256 entries.

Different interrupts have different interrupt numbers. With a IDT a CPU can look up a interrupt number and find the handler, for this specific event. For example the interrupt from a keyboard has the number 1. So the CPU searches in the IDT for a handler, which can process interrupts with the interrupt number 1 (a keyboard handler).

A entry in an IDT holds in which **segment** the interrupt service routine is locatet and the **offset** to the routine.

**[NOT FINSIHED]**
## Jump to Protected mode
**[Not Implemented]**
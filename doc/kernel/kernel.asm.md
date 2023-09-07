# Kernel
*This document is a basic Explenation what the Kernel (assembly) from the BobaOS does.*
## Job of the Kernel
The main job of the kernel is to finally make the jumt to c.
But first its sets up a:
 - GDT (Again so that we have all the resources in a single place)
 - IDT
 - PIT
 - PIC
 - Jump to ring3 and userspace
 - TSS

 ## GDT
 The kernel reloads the GDT but also adds new ring3 permissions to it.
 ``` assembly
    Gdt64:
    dq 0                    ; First entry is empty
    dq 0x0020980000000000   ; Code - Ring0, Access bytes: P = 1; S= 1; E = 1; Flags: L = 1;
    dq 0x0020f80000000000   ; Code - Ring3, Access bytes: P = 1; S = 1; E = 1; DPL = 3; Flags: L = 1;
    dq 0x0000f20000000000   ; Data - Ring3, Access bytes: P = 1; S = 1; E = 0; DPL = 3; RW = 1; Flags: L = 1;
```

 ## IDT
 The IDT handels all the interrupts. First 31 Interrupt vectors are reserved. 32 - 255 can be defined by us.

### Set Up IDT
 ``` assembly
 Idt:
    %rep 256
        dw 0
        dw 0x8      ; Points to the Code - Ring0 Entry in gdt
        db 0
        db 0x8e     ; Set attibutes to: P = 1; DPL = 0; Gate Type = 1110 (64 bit Interrupt Gate)     
        dw 0
        dd 0
        dd 0
    %endrep
```
IDT needs to have 255 entrys, even if they are not catched by us just jet.
(The IdtPtr struct is same as GdtPtr struct)
### Set Up Handler for vector 0 (devided by zero)
Here we are going to see, how we can create a interrupt handeling method.

We create a handleing method for vector0. (Divide by zero)

First we need to write the offset of the handler in the IDT.
The offset is devided into 3 parts in a IDT entry.
```
 127         96 95          64 63      48 47    16 15          1 0
  |  ........  |   Offset_3   | Offset_2 |........|  Offset_1   |
```
So we need to move the firs 2 bytes of the address of the handler in Offset_1.
The next 2 bytes in Offset_2 and the last 4 bytes in Offset_3.
``` assembly
    mov rdi, Idt            ; Move address of IDT in rdi
    mov rax, handler0       ; Move address of handler0 to rax

    mov[rdi], ax            ; Copy lower 16 bit of address in Offset_1
    shr rax, 16             ; Now the next 16 bit of handler0 address are in ax.
    mov[rdi+6], ax          ; Move ax into Offset_2. (+6 bytes = 48bit)
    shr rax, 16             ; Shift againt
    mov [rdi+8], eax        ; Write last 4 bytes in Offset_3

    lidt[IdtPtr]            ; Loads the IDT
```
Create the handler
``` assembly
handler0:
    ; This handler does nothing except it prints D in red on the screen

    mov byte[0xb8000], 'D'
    mov byte[0xb8001], 0xc

    jmp End     ; Stop system by jumping to end
    iret        ; Interrupt return is a speciel return for interrupts.
                ; It can return to a different priviledges level. (More later) 
```
Text your interrupt routing by dividing through zero
``` assembly
    xor rbx, rbx
    div rbx
```
## PIC & PIT
Please see [Hardwareinterrupts Here](../boot/HardwareInterrupts.md)

### Timer Handler
Now that we have set up a PIC and PIT we need to create a handler for the timer event.
``` assembly
    mov rdi, Idt            ; Move address of IDT in rdi
    add rdi, 32*16          ; Each entry in IDT is 16bit. We want to write handler for vector32. Sp 32*16
    mov rax, Timer          ; Move address of Timer to rax

    sti                     ; Set interrupt falg. Enables Hardware interrupts

    ; Now fill idt offset with address in rax.
    ; Same as in handler0
```
Now the Handler for the Timer
``` assembly
Timer:
    ; We save all the important register on the stack and pop them after we finished the Handler.
    ; With this we can manipulate these register and later when we return we can restor the state
    ; of the CPU.
    push rax
    push rbx
    push rcx
    push rdx
    push rsi
    push rdi
    push rbp
    push r8
    push r9
    push r10
    push r11
    push r12
    push r13
    push r14
    push r15

    mov byte[0xb8010], 'T'
    mov byte[0xb8011], 0xe

    pop r15
    pop r14
    pop r13
    pop r12
    pop r11
    pop r10
    pop r9
    pop r8
    pop rbp
    pop rdi
    pop rsi
    pop rdx
    pop rcx
    pop rbx
    pop rax

    iretq
```
## Jump to ring3 
We already set the 2 ring3 descriptors in in GDT. Now we jump with interrupt return form ring0 to ring3.
For this jump to work the CPU expects 5 values on the Stack:
 - RIP: Where we will return    <- Top of stack
 - Code Segment selector. (Gets loaded into cs register)
 - Rflags: Status of the CPU (Gets loaded into rflag register)
 - Stackpointer
 - Date Segment selector (Gets loaded in ss register)
 ``` assembly
    push 0x1B       ; Data selector 0000 0000 0001 1011 = Selected Index = 3 (Data for ring3); Ti=0; RPL=3;
    push 0x7c00     ; Stackpointer
    push 0x202      ; Rflags CPU. Bit 1 = 1 because required. Bit 9 = 1 interrupts are enabled
    push 0x13       ; Code Selector 0000 0000 0001 0011 = Selected Index = 2 (Code for ring3); TI=0; RPL=3;
    push UserEntry  ; Return point of iret.
```
## Task State Segment (TSS)
The use of the TSS in **long mode** is to change the stack pointer after a permission level change.
For example when we change permission for a hardware interrupt from ring0 to ring3 then the stack pointer is filled with null by the CPU.
With a TSS we can load the stackpointer for the current permission level.

TSS can do some more crazier stuff... but this is not needed normaly.
| 0x3 | 0x2 |0x1 | 0x0 |Offset |
| ------ | ------ | ------ | ------ | ------ |
| IOPB | Reserved | 0x64 |
|  Reserved | 0x60 |
|  Reserved | 0x5C |
|  IST7 | 0x54 |
|  IST6 | 0x4C |
|  IST5 | 0x44 |
|  IST4 | 0x3C |
|  IST3 | 0x34 |
|  IST2 | 0x2C |
|  IST1 | 0x24 |
|  Reserved | 0x20 |
|  Reserved | 0x1C |
|  RSP2 | 0x14 |
|  RSP1 | 0x0C |
|  RSP0 | 0x04 |
|  Reserved | 0x00 |

``` assembly
Tss:
    dd 0            ; Reserved
    dq 0x150000     ; RSP
    times 88 db 0   ; Set other fieldy to 0
    dd TssLen       ; IOPB = IO permission map Set it to size of TSS = IOBP is not used

TssLen: equ $ - Tss
```

### Add TSS in GDT
Now we need to Add the TSS in the GDT. The TSS entry in GDT is special, because it has the length of 2 entrys and a different structure.
See section [Data in a Table Entry (TSS)](../boot/Loader.md)
``` assembly
                    ; TSS Entry in the GDT
    dw TssLen - 1   ; Limit
    dw 0            ; Base (change later on runtime)
    db 0            ; Base
    db 0x80         ; Attribute (System): P = 1; DPL = 0; S = 0 (is System); Type = 0x9: 64-bit TSS available
    db 0            ; Flags + Limit
    db 0            ; Base
    dq 0            ; Base
```

Now we need to set the Base(Address) of the TSS in the GDT.
``` assembly
    mov rax, Tss            ; Move address of TSS to rax

    mov[TssDesc+2], ax      ; Set bit 0 - 15 from TSS address in GDT first base.
    shr rax, 16
    mov[TssDesc+4], al      ; Set bit 16 - 24 from TSS address in GDT second base.
    shr rax, 8
    mov[TssDesc+7], al      ; Set bit 25 - 32 from TSS address in GDT second base.
    shr rax, 8
    mov[TssDesc+8], rax     ; Set bit 33 - 64 from TSS address in GDT second base.

    mov ax 0x20             ; Selected Index = 4; TI = 0; RPL = 0;
    ltr ax                  ; Load TSS with this command.
```

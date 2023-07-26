# Loader
*This document is a basic Explenation what the loader from the BobaOS does.*
## Job of the bootloader
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
```x86asm
mov eax, 0x8000000
cpuid
cmp eax, 0x80000001
```
If eax is less than 0x80000001 that means that exteded function is not available.
If extended function is available check for long mode.
```x86asm
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
```x86asm
; Note - please dont write in random memory addresses. This is just a example.
mov ax, 0xFFFF
mov es, ax
mov word[0x80C8], 0xa200
mov word[ds:0x80D8], 0xb200     ; 0xFFFF * 16 + 0x80D8 = 0x1080C8
cmp word[0x80C8], 0xb200
je A20IsOffErr
```

## Set Video mode 
We set video mode to text.
After we made the jump to protect mode, we cannot write to the screen anymore through BIOS functions, because they are not available anymore. 

In Video mode we have 20 rows and 80 columns we cann fill with characters. One Character is 2 byte. First byte is ascii char value, the second byte is char attribute.
The first 4 bit of the attribute byte are forground color. The second 4 bit are background color.

Video memory starts at 0xb8000.
We write to the screen, by just writing the bytes in video memory.
Enable video mode:
```x86asm
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
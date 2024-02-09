# A20 line
The A20 line represents the 20st line on a address bus (counting from 0). The old 8086 had a memory wraparound feature where addresses larger than 1MByte got truncated. 
That newer processor can still be combatible with old programs who use this feature the A20 line support was added.

A20 Line needs to be **on**. Because with A20 line, we can access the 20th bit.
If A20 Line is off addresses that uses the 20th bit gets truncated.
```
0x1080C8    -   1 0000 1000 0000 1100 1000
       truncated to
0x0080C8    -   0 0000 1000 0000 1100 1000
```

## Test for A20 line support
*This example test needs to be done in real mode.*

We check if A20 Line is on, by writing a value to 0x0080C8. 
Then we write a **different** value to 0x1080C8.

If we now look at 0x0080C8 and check if the value is the same value we wrote in 0x1080C8, that means that A20 Line is off, because 0x1080C8 got truncatet and overwrote 0x0080C8.

```assembly
; Note - please dont write in random memory addresses. This is just a example.
mov ax, 0xFFFF
mov es, ax
mov word[0x80C8], 0xa200
mov word[es:0x80D8], 0xb200     ; 0xFFFF * 16 + 0x80D8 = 0x1080C8
cmp word[0x80C8], 0xb200
je A20IsOff
```

## Enable A20 line support
There are different ways to enable A20 line support.

### Fast A20 gate
Many modern processors have a Fast A20 option that can quickly enable the A20 line support.

*Note: This method is not recommend, because it is not 100% save that this option is supported by the current CPU*
```assembly
in al, 0x92
or al, 2
out 0x92, al
```

### INT 15
On many CPUS it is supported to activate A20 line support trough the BIOS function.
```assembly
mov ax, 0x2403					; Command to see if this action is supported
int 0x15
cmp ah, 0					; If ah is not 0 or the carry flag is set, than activating A20 trough int 0x15 is not supported.

mov ax, 0x2401					; Command to activate A20 line support
int 0x15						
``` 
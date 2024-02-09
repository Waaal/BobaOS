# Booting trough BIOS
When booting with the BIOS the BIOS searches all of the devices (boot order in BIOS settings) for a bootable device and if it findes one the first 512 bytes are loaded into memory address 0x7C00. After loading 512 bytes into memory it begins executing at address 0x7C00.

A device is bootable if the last 2 bytes from 512 has the signature 0x55 0xAA.
When the BIOS boots the CPU is in real mode (16 bit).

Before the BIOS begins executing our bootloader it writes the driveId into the **dl** register.

## BIOS Parameter block
Our bootloader needs to be exactly 512 bytes long and the last 2 bytes need to be the boot signature. But we still cannot use all of the 510 bytes for code. 
Some BIOSes expects a BIOS parameter block at the beginning of our Bootloader. This block is 36 bytes long. 

Struct of the BIOS parameter block:

| Offset | Size (in bytes) |Descritpion |
| ------ | ------ | ------ |
| 0x00 | 3 | JMP SHORT 3C NOP |
| 0x03 | 8 | OEM identifier. Used by microsoft example: MSWIN4.1. Can now be any string with the length of 8 |
| 0x0B | 2 | Numbers of bytes per sector. Normaly 512 |
| 0x0D | 1 | Numbers of sectors per cluster |
| 0x0E | 2 | Numbers of reserved secors |
| 0x10 | 1 | Number of File allocation tables |
| 0x11 | 2 | Number of Root Directory entrys |
| 0x13 | 2 | Number of Total sectors. If 0 then number is greater than 65535 and is stored in Large sector count (0x20) |
| 0x15 | 1 | Media Type signature (0x0F0 = Floppy disk) |
| 0x16 | 2 | Numers of sectors per FAT. Means how long is File allocation table in sectors (FAT12/16 only) |
| 0x18 | 2 | Numbers of sectors per Track |
| 0x1A | 2 | Numbers of heads/sides of storage media |
| 0x1C | 4 | Numbers of hidden sectors |
| 0x20 | 4 | Large sector count |

The only important info in the BIOS parameter block are the firs 3 bytes. This will jump to the *real* entry of our bootloader. The others can be ignored, but cannot be used for our code because some BIOSes may write values into the BIOS parameter block. If we would have code at this position it would be overwritten possibly breaking our bootloader.

## Barebon bootloader setup
Our bootloader:
| Offset | Bytes |Descritpion |
| ------ | ------ | ------ |
| 0 | 36 | BIOS parameter block |
| 36 | 474 | Code |
| 510 | 2 |  0x55 0xaAA bootable signature|

Here is a barebone bootloader that would boot on a x86 CPU machine.
``` assembly
[ORG 0x7C00]
[BITS 16]					; Becaue CPU is in real mode

_start:
	jmp short start
	nop
	times 33 db 0

						; Placeholder for BIOS parameter block, because some BIOSes want this and write in this block

start:
	jmp 0x0:$+1				; Set code segment to 0x0, because it cannot be set with mov cs. So set it with a jump.

	cli					; Clear Interrupts and disable
						; We disable interrupts bevore we change the segment registers, because this is a critical operration and we ; dont want that a interrupt occure while we doing this
	mov ax, 0x0
	mov es, ax
	mov ds, ax
	mov ax, 0x00
	mov ss, ax
	mov sp, 0x7c00			; We changed all segment registers to 0, because some BIOSes might set them to 0x7C0, some to 0 and some 
						; dont set them at all. So it is good practice to set them to 0, or to 0x7c0 if we dont want to
						; use org 0x7c00

	sti					; Enables Interrupts	

times 510- ($ - $$) db 0	; Fill up the size of the bootloader to 510 bytes
dw 0xAA55				; Intel = little endian, dw gets flipped. So dw AA55 = 0x55 0xAA
```
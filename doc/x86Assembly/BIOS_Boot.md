# Booting trough BIOS
When booting with the BIOS the BIOS searches all of the devices (boot order in BIOS settings) for a bootable device and if it findes one the first 512 bytes are loaded into memory address 0x7C00. After loading 512 bytes into memory it begins executing at address 0x7C00.

A device is bootable if the last 2 bytes from 512 has the signature 0x55 0xAA.
When the BIOS boots the CPU is in real mode (16 bit).

Before the BIOS begins executing our bootloader the BIOS writes the driveId into the **dl** register.

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

The only important info in the BIOS parameter block are the firs 3 bytes. This will jump to the *real* entry of our bootloader. The others can theoretically be ignored. But some BIOSes only boot if we have a valid BIOS parameter block. So it is good practise to have a valid BIOS parameter block. When our OS begins to grow we want to load stuff from disk anyway, so at this point we ned to have a valid BIOS parameter block for our file system. But anyway we cannot use the BPB for our code because some BIOSes may write values into the BIOS parameter block. If we would have code at this position it would be overwritten possibly breaking our bootloader.

## Partition Table
The partition table is only needed for hard disk drives. The partition table is a traditional way of storing information about the hard disk. The MBR and the partition tables are nearly obsolete by now but almost all PCs and older PCs still use this for booting. So we need to have a valide Partition Table entry in our bootloader. 


The system maybe boots without a valid partition table but if we try to access disk operations with BIOS int 13 and others it will fail on some BIOSes without a valid parition table.


There are 4 partition tables at the end of the bootloader. We only need one and can fill the others with 0. One partition table has 16 bytes.


**All 4 partition tables offsets**
| Partition number | Offset |
| ------ | ------ | 
| 1 | 446 | 
| 2 | 462 |
| 3 | 478 |
| 4 | 494 |


**partition table struct**
| Offset | Size | Descritpion |
| ------ | ------ | ------ |
| 0 | 1 byte | Boot indicator 0 = no, 0x80 = bootable |
| 1 | 1 byte | Starting head |
| 2 | 6 bits | Starting sector | 
| 3 | 10 bits |  Starting cylinder |
| 4 | 1 byte |  System ID (what file system this disk contains) |
| 5 | 1 byte |  Ending head |
| 6 | 6 bits |  Ending sector |
| 7 | 10 bits |  Ending cylinder |
| 8 | 4 byte |  Relative sector |
| 12 | 4 bytes | Total sectors in partition | 


The only important values are the Boot indicator, the system ID and the Total sectors in partition. We normally set the boot indicator to bootable, the System ID to the right file system and the Total sectors to something like 65535. The total secors are used by the BIOS for the CHS math. So we need to have enough sectors for the CHS int to load our kernel etc. So at best set it to the same value as the BRB total sectors.

## Barebon bootloader setup
Our bootloader:
| Offset | Bytes | Descritpion |
| ------ | ------ | ------ |
| 0 | 36 | BIOS parameter block |
| 36 | 474 | Code |
| 446 | 64 | 4 x Partition Table | 
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
	jmp 0x0:next				; Set code segment to 0x0, because it cannot be set with mov cs. So set it with a jump.
.next:

	cli					; Clear Interrupts and disable
						; We disable interrupts bevore we change the segment registers, because this is a critical operration and we ; dont want that a interrupt occure while we doing this
	mov ax, 0x0
	mov es, ax
	mov ds, ax
	mov di, ax
	mov fs, ax
	mov ss, ax
	mov sp, 0x7c00				; We changed all segment registers to 0, because some BIOSes might set them to 0x7C00, some to 0 and some 
						; dont set them at all. So it is good practice to set them to 0, or to 0x7c00 if we dont want to
						; use org 0x7c00

	sti					; Enables Interrupts	

times 446- ($-$$) db 0 			; Fill up the bootloader to the start of the partition table

partition_table:
	db 0x80				; Bootable FLAG 0x80 = bootable
	db 0x00				; Starting head
	db 0x00				; Starting sector and cylinder
	db 0x00				; staring cylinder
	db 0x0E				; Partition type (System ID)
	db 0x00				; Enbding head
	db 0x00				; Ending sector 6bit
	db 0x00				; Ending sector 10 bit
	dd 0x00				; Relative sector 32 bit (4 byte)
	dw 0xFFFF			; Total sectors 32 bit (4 bytes)
	dw 0x0
times 48 db 0
dw 0xAA55				; Intel = little endian, dw gets flipped. So dw AA55 = 0x55 0xAA
```
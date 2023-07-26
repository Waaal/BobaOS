# Bootloader
*This document is a basic Explenation what the Bootloader from the BobaOS does*
## Job of the bootloader
Our bootloader does 2 things:

It checks if LBA (Logical Block Addressing) is enabled, because we use LBA to load the next files. If LBA is not supportet the Bootloader stops and prints a error message.

It loads the Loader (Second step of the bootloader).


## Basic
 - Has to be exactly **512** bytes long. 
 - The last 2 bytes need to be 0xaa 0x55, so that the BIOS, which loads the Bootloader, knows that it is    bootable.
 - BIOS always load bootloader into memory address 0x7c00.
 - CPU is in real mode. Only 16 bytes and we still can use BIOS calls.

## Byte distribution
512 bytes in toal
| Offset | Bytes |Descritpion |
| ------ | ------ | ------ |
| 0 | 440 | Code |
| 440 | 4 | Signature |
| 444| 2 | NULL |
| 446 | 64 | 4*16 byte Partition entrys |
| 510 | 2 |  aah 55h bootable signature|

### Partition entry description
16 bytes in total
| Offset | Bytes |Descritpion |
| ------ | ------ | ------ |
| 0 | 1 | Status or physical drive |
| 1 | 3 | CHS value of first absolute sector |
| 4 | 1 | Partition Type |
| 5 | 3 | CHS value of last absolute sector |
| 8 | 4 | LBA of First sector |
| 12 | 4 | Numbers of sectors |
## Check for LBA
```assembly
mov ah, 0x41
mov bx, 0x55aa
int 0x13            ; Interrupt for Low Level Disk Services
```
If there is a read error with the disk service, the carry bit is set.
If after interrupt bx is not equal 0xaa55 then LBA is not supported
## Load Loader
*Small explenetion on how to load with LBA*
```assembly
ReadPackage:
	db 0x10	; Length of ReadPackage struct
	db 0		; Always Zero
	dw 5		; Numbers of Sectors we want to read from the img file
	dw 0x7e00	; Destination RAM address
	dw 0		; in memory page zero
	dd 1		; img file read start sector
	dd 0		; more storage bytes
```
We read 5 sectors from the img file starting at sector 1 (Sector 0 was bootloader).
We load it directly above the Bootloader in RAM (Bootloader 0x7c00 + 512 = 0x7e00).
```assembly
	mov si, ReadPackage
	mov ah, 0x42        ; Code, that we want to use discExtensionService
	mov dl, [driveId]   ; From which drive we want to load
	int 0x13            ; Interrupt for Low Level Disk Services
```

### Worth to mention "driveId"
Before the BIOS calls our bootloader. He writes the driveId, where our image file is located, into the **dl** register. We safe this into the "driveId" variable, because we need this id every time we want to read from our img file.

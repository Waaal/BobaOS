[bits 16]
[org 0x7C00]

	jmp short start
	nop

; FAT32 header
OEMIdentifier			db 'BOBAV0.1'
BytesPerSector			dw 0x200		; 512 bytes
SectorsPerCluser 		db 0x1			; 1 sectors per cluster
ReservedSectors			dw 240			; Store kernel in the reserved sectors
FATCopies 				db 0x02
RootDirEntries			dw 0x00			; 0 for fat32
NumSectors				dw 0x00
MediaType				db 0xF8
SectorsPerFat12_16		dw 0			; 0 for fat32
SectorsPerTrack			dw 0x20	
NumberOfHeads			dw 0x04
HiddenSectors			dd 0x00
SectorsBig 				dd 0x10000		; 32 MB

; FAT32 extended
SectorsPerFat32			dd 0x200		; (numClusters * 4) / sizeOfSector (512) (also minus reserved and actuall FAT because the FAT only holds data clusters but eh)
Flags					dw 0
FatVersion				dw 0
RootDirCluster			dd 2 			; Cluster 2 is always start of data cluster (hard coded in FAT standarts)
										; Start of data cluster (2): reserved 200 + (2*FAT = 1024) = 1224 = start of data cluster. (Next cluster = 1225,1226 etc)
FsInfoSector			dw 1
backUpBoot				dw 6			; We dont have one... but fuck it
times 12 db 0							; reserved
DriveNumber				db 0x80
WinNTBit				db 0x00
Signature 				db 0x29
VolumeID				dd 0xD106
VolumeIDString 			db 'BOBAOS BOOT'
SystemIDString 			db 'FAT32   '

start:
	jmp 0x0:next			; set CS register to 0

next:
	cli
	
	mov [driveId], dl
	
	xor ax, ax
	xor dx, dx
	xor cx, cx 

	mov ax, 0x0				; set segment registers to 0
	mov es, ax
	mov ds, ax
	mov ss, ax
	mov fs, ax
	mov gs, ax
	mov sp, 0x7c00

	mov di, ax
	mov si, ax

enableA20:
	in al, 0x92
	or al, 2
	out 0x92, al

testA20Line:
	mov ax, 0xFFFF
	mov es, ax

	mov word[0x80C0], 0xA200
	mov word[es:0x80D0], 0xB200
	
	cmp word[0x80C0], 0xA200
	je checkExtendedRead

	mov si, a20LineErrorMsg
	call print

	jmp $

checkExtendedRead:
	mov ah, 0x41
	mov bx, 0x55aa	
	mov dl, [driveId]
	
	int 0x13
	
	jnc readLoader
	
	mov si, extendedReadErrorMsg 
	call print

	jmp $

readLoader:	
	mov si, extreadStage2Package
	call extendedRead

changeTextMode:
	mov ah, 0x00
	mov al, 0x03
	int 0x10

	mov dl, [driveId]
	jmp 0x8000

; Print function
; Expects:
;			- Pointer to string in si register
; Modifies: ax, bx, si
print:
	xor ax, ax
	xor bx, bx
	mov ah, 0xE
	mov bh, 0
.loop:
	lodsb
	cmp al, 0
	jz .done
	int 0x10
	jmp .loop
.done:
	ret

; Extended read function
; Expects:
;			- Pointer to readPackage si
;			- Pointer to error message string in di
; Modifies: ax, dx, si, di
extendedRead:
	xor ax, ax
	xor dx, dx 
	mov ah, 0x42
	mov dl, [driveId]

	int 0x13	
	jnc .end
	
	;TODO print
	;read error

	jmp $
.end:
	ret

driveId: db 0
extendedReadErrorMsg: db 'Extended read is not supported',0
a20LineErrorMsg: db 'A20 line is off. Please enable A20 Line in BIOS',0

extreadStage2Package:
	dw 0x10					; Package size(0x10 or 0x16)
	dw 0x2					; Total LBA to laod
	dw 0x8000				; destination address(0x00:[0x8000])
	dw 0x0					; destination (segment [0x00]:0x8000)
	dd 0x2					; starting LBA in our img file
	dd 0x0					; more storage bytes for bigger lbas

times 446 - ($-$$) db 0

partitionTable:
	db 0x80			; bootable flag
	db 0x00			; starting head
	db 0x00			; starting sector and cylinder
	db 0x00			; starting cylinder
	db 0x0E			; partition type
	db 0x00			; ending head
	db 0x00			; ending sector 6bit
	db 0x00			; ending sector 10 bit
	dd 0x00			; relative sector 32 bit
	dd 0x10000		; Total sectors 32 bit (4 bytes)

times 48 db 0


	dw 0xAA55



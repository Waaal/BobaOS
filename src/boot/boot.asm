[bits 16]
[org 0x7C00]

	jmp short start
	nop
	times 33 db 0			; BIOS parameter block placeholder

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
	mov di, readErrorMsg
	call extendedRead

readKernel:
	mov si, extreadKernelPackage
	call extendedRead

changeTextMode:
	mov ah, 0x00
	mov al, 0x03
	int 0x10

; Zero out 1000 bytes for the memory map
zeroBytes:	
	xor ax,ax
	mov es, ax
	mov di, 0x8000
	mov cx, 1000
	rep stosd

; for this BIOS int we need to use 32 bit registers lol
getMemoryMap:
	xor ebx, ebx
	mov es, bx
	mov edi, 0x8000
	mov ecx, 20

.loop:
	mov eax, 0xE820
	mov edx, 0x534D4150		;'SMAP' 

	int 0x15
	jc .error

	cmp ebx, 0
	jz .done

	add edi, 20
	jnc .loop
.error:
	mov si, memoryMapErrorMsg
	call print
	jmp $
.done:
	jmp 0x7E00

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
	
	mov si, di
	call print

	jmp $
.end:
	ret
	
	; TODO
	; 	Partition table

driveId: db 0
extendedReadErrorMsg: db 'Extended read is not supported. Booting stopped',0
readErrorMsg: db 'There was an error reading from disk',0
a20LineErrorMsg: db 'A20 line is off. Please enable A20 Line in BIOS',0
memoryMapErrorMsg: db 'There was an error reading the memory map',0

extreadStage2Package:
	dw 0x10					; Package size(0x10 or 0x16)
	dw 0x2					; Total LBA to laod
	dw 0x7E00				; destination address(0x00:[0x7E00])
	dw 0x0					; destination (segment [0x00]:0x7E00)
	dd 0x1					; starting LBA in our img file
	dd 0x0					; more storage bytes for bigger lbas

extreadKernelPackage:
	dw 0x10					; Package size(0x10 or 0x16)
	dw 100					; Total LBA to load
	dw 0x0					; destination address(0x00:[0x00])
	dw 0x1000				; destination (segment [0x1000]:0x00)
	dd 0x3					; starting LBA in our img file
	dd 0x0					; more storage bytes for bigger lbas

	times 510-($-$$) db 0
	dw 0xAA55



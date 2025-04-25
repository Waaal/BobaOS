[bits 16]
[org 0x7E00]

prepare32bit:
	lgdt [cs:gdt_ptr32]

	mov eax, cr0
	or eax, 0x1
	mov cr0, eax

	jmp 0x8:pml

[bits 32]
pml:
	;Set data segments to 0x10
	mov eax, 0x10
	mov es, eax
	mov ds, eax
	mov ss, eax
	mov fs, eax

	mov esp, 0x7c00

	xor eax, eax
	mov ax, 'T'
	mov ah, 0x3

	mov [0xb8000], ax
	
pagingSetup:	
	;Zero locations for our 2 page tables
	xor eax,eax
	mov edi, 0x50000
	mov ecx, 0x10000/4
	rep stosd
		
	;Setup PLM4 at location 0x50000 and PDP at location 0x51000 and do a 1GB 1 to 1 mapping
	mov dword[0x50000], 0x51003			; P = 1, RW = 1
	;mov dword[0x50000+0x800], 0x51003	; P = 1, RW = 1	
	
	mov dword[0x51000], 0x83			; P = 1, RW = 1, PS = 1

enablePaging:
	;Enables Phyiscal address extension
	mov eax, cr4
	or eax,	(1<<5)
	mov cr4, eax
	
	;Set cr3 to first page table 
	mov eax, 0x50000
	mov cr3, eax

	; go into 64 bit compability mode
	mov ecx, 0xC0000080
	rdmsr
	or eax, (1<<8)
	wrmsr

	;Enable paging
	mov eax, cr0
	or eax, (1<<31)
	mov cr0, eax
	
	;load GDT to switch to long mode and no long/compability mode
	lgdt[gdt_ptr]
	jmp 0x8:longEntry

[bits 64]
longEntry:
	mov rsp, 0x200000

	xor rax, rax

PICSetup:
	; ICW_1
	xor eax, eax
	mov al, 00010001b
	out 0x20, al
	out 0xa0, al

	; ICW_2
	mov al, 32
	out 0x21, al
	mov al , 40
	out 0xa1, al

	; ICW_3
	mov al, 4
	out 0x21, al
	mov al, 2
	out 0xa1, al

	; ICW_4
	mov al, 1
	out 0x21, al
	out 0xa1, al

moveKernel:
	mov rsi, 0x10000
	mov rdi, 0x100000
	mov rcx, 51200/8			; 100 sectors	

	rep movsq

	mov ax, 'L'
	mov ah, 0x4

	mov [0xb8000], ax

	jmp 0x100000


; A GDT entry in protected mode is 8 byte
gdt32:
gdtnull32:	dq 0x0
gdtcode32:	dw 0xFFFF		; 2 byte limit
			dw 0x0
			db 0x0			; 3 byte base
			db 10011010b	; 1 byte access bytes P = 1, DPL = 00, S = 1 (Code/Data), E = 1 (code), DC = 0, RW = 1, A = 0 (leave it 0 because the CPU is going to use this)
			db 11001111b	; 4 bit Flags 4 bit Limit | Flags G = 1, DB = 1 (protected mode)
			db 0x0			; 1 byte base 

gdtdata32:	dw 0xFFFF		; 2 byte limit
			dw 0x0
			db 0x0			; 3 byte base
			db 10010010b	; 1 byte access bytes P = 1, DPL = 00, S = 1 (Code/Data), E = 0 (data), DC = 0, RW = 1, A = 0 (leave it 0 because the CPU is going to use this)
			db 11001111b	; 4 bit Flags 4 bit Limit | Flags G = 1, DB = 1 (protected mode)
			db 0x0			; 1 byte base
gdt_len32: equ $ - gdt32

; GDT pointer 6 bytes (2 bytes len, 2 bytes address)
gdt_ptr32:
	dw gdt_len32-1
	dw gdt32

; GDT Long mode
gdt:
gdtnull: dq 0					; GDT Null entry
gdtcode: dq 0x00209A0000000000 	; GDT Code
gdtdata: dq 0x0020920000000000	; GDT Data

gdt_len: equ $ - gdt

; GDT ptr (2 byte len, 4 byte address)
gdt_ptr: dw gdt_len-1
		 dq gdt

times 0x400  - ($-$$) db 0x00

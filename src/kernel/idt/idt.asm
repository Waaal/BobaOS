[bits 64]

extern trapHandler

global idtAddressList

global enableInterrupts
global disableInterrupts
global loadIdtPtr

global DEBUG_STRAP_REMOVE_LATER

section .text.asm

; just for testing a single interrupt (32 - timer) purpose
DEBUG_STRAP_REMOVE_LATER:
	mov rax, int32Test
	mov [temp1], rax
	ret

enableInterrupts:
	sti	
	ret

disableInterrupts:
	cli
	ret

; void loadIdtPtr(struct idtPtr* ptr)
loadIdtPtr:
	lidt [rdi]
	ret

saveRegisters:	
	mov [tempSave], rax
	pop rax	; pop the return address
	mov [saveRetAddress], rax
	mov rax, [tempSave]

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

	mov rax, [saveRetAddress]
	jmp rax

loadRegisters:
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
	
	add rsp, 8
	jmp $	
	iretq

int32Test:
	; ----------------------	<----- Start if we come from ring3
	; DATA SEGMENT
	; STACK POINTER
	; ----------------------    <----- Start if we come from ring0
	; EFLAGS
	; CODE SEGMENT
	; IP 						<----- CURRENT STACK POINTER
	
	sub rsp, 8 ;Bring back 16 bit alginment

	call saveRegisters

	mov rdi, 32
	mov rsi, rsp
	mov rdx, 1

	call trapHandler
	
	jmp loadRegisters

section .data

saveRetAddress:
	resq 1
tempSave:
	resq 1

sat:
	resq 5

idtAddressList:
	resq 32
	temp1

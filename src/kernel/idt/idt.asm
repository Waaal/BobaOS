[bits 64]

extern trapHandler

global idtAddressList

global enableInterrupts
global disableInterrupts
global loadIdtPtr

section .text.asm

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
	iretq

; Create a NASM macro so that we dont have to create a interrupt wrapper for each interrupt
%macro intertupt 1
int%1:
	; ----------------------	<----- Start if we come from ring3
	; DATA SEGMENT
	; STACK POINTER
	; ----------------------    <----- Start if we come from ring0
	; EFLAGS
	; CODE SEGMENT
	; IP 						<----- CURRENT STACK POINTER

	sub rsp, 8 ;Bring back 16 bit alginment

	call saveRegisters

	mov rdi, %1
	mov rsi, rsp

	call trapHandler	
	jmp loadRegisters
%endmacro

; Create for loop 256 times macro interrupt i
; So we have 256 interrupt handelers
%assign i 0
%rep 256
	intertupt i
%assign i i+1
%endrep

section .data

saveRetAddress:
	resq 1
tempSave:
	resq 1
sat:
	resq 5

%macro idtListEntry 1
	dq int%1
%endmacro

idtAddressList:
%assign i 0
%rep 256
	idtListEntry i
%assign i i+1
%endrep




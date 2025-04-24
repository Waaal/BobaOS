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

; Create a NASM macro to create a interrupt wrapper for each interrupt
%macro interrupt 1
int%1:
	; DATA SEGMENT
	; STACK POINTER
	; EFLAGS
	; CODE SEGMENT
	; IP 						<----- CURRENT STACK POINTER

	sub rsp, 8 ;Bring back 16 bit alginment

	call saveRegisters

	mov rdi, %1
	xor rsi,rsi	; No error code by CPU 
	mov rdx, rsp

	call trapHandler	
	jmp loadRegisters
%endmacro

; Create a NASM macro for exceptions where CPU places error code on stack
%macro interruptErrorCode 1
int%1:
	; DATA SEGMENT
	; STACK POINTER
	; EFLAGS
	; CODE SEGMENT
	; IP 						<----- CURRENT STACK POINTER
	
	pop rsi ; Error code placed by cpu as first argument
	sub rsp, 8 ;Bring back 16 bit alginment

	call saveRegisters

	mov rdi, %1
	mov rdx, rsp

	call trapHandler	
	jmp loadRegisters
%endmacro

; Create all 256 interrupt routines wrapper
%assign i 0
%rep 8
	interrupt i
%assign i i+1
%endrep
interruptErrorCode 8
interrupt 9
interruptErrorCode 10
interruptErrorCode 11
interruptErrorCode 12
interruptErrorCode 13
interruptErrorCode 14
interrupt 15
interrupt 16
interruptErrorCode 17
interrupt 18
interrupt 19
interrupt 20
interruptErrorCode 21 
interrupt 22
interrupt 23
interrupt 24
interrupt 25
interrupt 26
interrupt 27
interrupt 28
interruptErrorCode 29
interruptErrorCode 30
interrupt 31
%assign i 32
%rep 244
	interrupt i
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




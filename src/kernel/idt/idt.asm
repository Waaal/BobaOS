[BITS 64]

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

int32Test:
	jmp $
	iretq

section .data
idtAddressList:
	resq 32
	temp1

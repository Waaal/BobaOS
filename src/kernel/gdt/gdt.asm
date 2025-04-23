global loadGdtPtr 
extern gdtPtr

section .text.asm

;void loadGdtPtr(struct gdtPtr* ptr)
loadGdtPtr:
	
	mov rax, rdi
	lgdt [rax]
	
	push qword 0x08		; CS selector
	lea rax, [rel setGdt]
	push rax
	retqf
	
setGdt:
	ret

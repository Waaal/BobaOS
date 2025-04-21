global loadGdtPtr 
extern gdtPtr

section .text.asm

;void loadGdtPtr(struct gdtPtr* ptr)
loadGdtPtr:
	
	mov rax, rdi
	lgdt [rax]
	ret

global loadGdtPtr 
extern gdtPtr

section .text.asm

;void loadGdtPtr(struct gdtPtr* ptr)
loadGdtPtr:

lgdt [rdi]
ret


db "hello im in assembly code", 0

global loadGdtPtr 
extern gdtPtr

section .asm

;void loadGdtPtr(struct gdtPtr* ptr)
loadGdtPtr:

lgdt [rdi]
ret


db "hello im in assembly code", 0

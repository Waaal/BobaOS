global loadNewPageTable

section .text.asm

; void loadNewPageTable(PML4 table)
loadNewPageTable:
mov cr3, rdi 
ret

global loadNewPageTable

section .text.asm

; void loadNewPageTable(PLM4 table)
loadNewPageTable:
mov cr3, rdi 
ret

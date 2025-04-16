global loadNewPageTable

section .asm

; void loadNewPageTable(PLM4 table)
loadNewPageTable:
mov cr3, rdi 
ret

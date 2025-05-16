[bits 64]

global _start
extern kmain

_start:
    call kmain
    jmp $
times 512 - ($ - $$) db 0

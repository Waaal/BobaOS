[bits 64]

global _start

global testJmp
extern kmain

_start:

mov rsp, 0x200000
call kmain
jmp $

times 512 - ($ - $$) db 0

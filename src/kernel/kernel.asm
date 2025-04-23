[bits 64]

global _start

global divZeroTest

global testJmp
extern kmain

_start:

mov rsp, 0x200000
call kmain
jmp $

divZeroTest:

xor rdx, rdx
div rdx

jmp $


times 512 - ($ - $$) db 0

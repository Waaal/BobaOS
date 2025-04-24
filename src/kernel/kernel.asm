[bits 64]

global _start

global divZeroTest

global testJmp
extern kmain

_start:

call kmain
jmp $

times 512 - ($ - $$) db 0

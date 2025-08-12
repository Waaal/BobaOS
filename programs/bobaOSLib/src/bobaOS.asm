extern main
global _start

section .text

;int start()
_start:
    call main
    ret


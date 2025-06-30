[bits 64]

global loadTss

section .text.asm

;void loadTss(uint16_t descriptor)
loadTss:
    mov ax, di
    ltr ax
    ret
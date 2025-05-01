global outb
global outw
global outd

global inb
global inw
global ind

section .text.asm

;void outb(uint16_t port, uint8_t out) 
outb:
	mov rax, rsi
	mov rdx, rdi
	out dx, al 
	ret

;void outw(uint16_t port, uint16_t out) 
outw:
	mov rax, rsi
	mov rdx, rdi
	out dx, ax 
	ret

;void outd(uint16_t port, uint32_t out) 
outd:
	mov rax, rsi
	mov rdx, rdi
	out dx, eax 
	ret

;uint8_t inb(uint16_t port)
inb:
	xor rax, rax

	mov rdx, rdi
	in al, dx

	ret

;uint16_t inw(uint16_t port)
inw:
	xor rax, rax

	mov rdx, rdi
	in ax, dx

	ret

;uint32_t ind(uint16_t port)
ind:
	xor rax, rax

	mov rdx, rdi
	in eax, dx

	ret

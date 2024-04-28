# Linking Stdlib
We have 2 options to link our stdlib to userprograms. Static and dynamic. Thanks to ELF files dynamic linking is possible and every user program doesnt need to compile the stdlib into their binary. Static linking is compiling the stdlib in the binary.

## Important stdlib job
The stdlib doesnt only include usefull commands. It also contains a wrapper with the real start entry. This wrapper can then provide arguments etc and call the c main function. The entry point in the linker for every userpogram needs to be start for this to work.


Simple wrapper that doesnt have arguments etc.
``` asm
[bits 32]

global _start
extern main

section .asm

_start:
	call main
	ret
```

## Static

## Dynamic
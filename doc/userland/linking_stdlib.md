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
To statically link the stdlib we create a relocatbale elf file out of the stdlib files. This relocatable ELF file can then be used by a different linker (the linker of the user programm) and link the stdlib.elf file in.
``` makefile
FILES=./build/start.o ./build/bobaos.o

all: ${FILES}
	i686-elf-ld -m elf_i386 -relocatable ${FILES} -o ./bin/stdlib.elf

./build/start.o: ./src/start.asm
	nasm -f elf ./src/start.asm -o ./build/start.o

./build/bobaos.o: ./src/bobaos.asm
	nasm -f elf ./src/bobaos.asm -o ./build/bobaos.o
clean:
	rm -rf ${FILES}
	rm ./bin/stdlib.elf
```


**start.o:** Is the start routine with the entry label.


**bobaos.o:** The actuall stdlib commands


So we compile it with the -relocatable flag as a stdlib.elf file. Later the userpograms need to link this elf file in their executable. The linkerscript of the userpgoram needs to specify the start lable of the start.o as the entry point. 

## Dynamic
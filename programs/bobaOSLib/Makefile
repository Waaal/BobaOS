FILES=./build/bobaOSLib.asm.o
FLAGS= -g -ffreestanding -falign-jumps -falign-functions -falign-labels -falign-loops -fstrength-reduce -fomit-frame-pointer -finline-functions -Wno-unused-functions -fno-builtin -Wno-unused-label -Wno-cpp -Wno-unused-parameter -nostdlib -nostartfiles -nodefaultlibs -Wall -O0 -Werror

all: $(FILES)
	x86_64-elf-ar rsc ./lib/BobaOSLib.a $(FILES) 

./build/bobaOSLib.asm.o: ./src/bobaOS.asm
	nasm -f elf64 ./src/bobaOS.asm -o ./build/bobaOSLib.asm.o

clean:
	rm ./build/bobaOSLib.asm.o

FILES=./build/empty.o
INCLUDES= -I../stdlib/src -I../bobaOSLib/src 
FLAGS= -g -ffreestanding -falign-jumps -falign-functions -falign-labels -falign-loops -fstrength-reduce -fomit-frame-pointer -finline-functions -Wno-unused-functions -fno-builtin -Wno-unused-label -Wno-cpp -Wno-unused-parameter -nostdlib -nostartfiles -nodefaultlibs -Wall -O0 -Werror

all: $(FILES)
	x86_64-elf-gcc -g -T ./src/empty_linker.ld $(FILES) ../BobaOSLib/lib/bobaOSLib.a -o ./bin/empty.bin -ffreestanding -O0 -nostdlib -fpic -g

./build/empty.o: ./src/empty.c
	x86_64-elf-gcc -std=gnu99 $(INCLUDES) $(FLAGS) -c ./src/empty.c -o ./build/empty.o

clean:
	rm ./build/empty.o

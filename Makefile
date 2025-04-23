FILES= ./build/kernel.asm.o ./build/kernel.o ./build/memory.o ./build/kheap.o ./build/gdt.o ./build/gdt.asm.o ./build/paging.o ./build/paging.asm.o ./build/terminal.o ./build/string.o ./build/io.asm.o ./build/idt.o ./build/idt.asm.o ./build/irqHandler.o ./build/exceptionHandler.o
INCLUDE= -I ./src/kernel/
FLAGS= -g -ffreestanding -mcmodel=kernel -fno-pic -fno-pie -mno-red-zone -falign-jumps -falign-functions -falign-labels -falign-loops -fstrength-reduce -fomit-frame-pointer -Wno-unused-parameter -finline-functions -fno-builtin -Wno-cpp -nostdlib -nostartfiles -nodefaultlibs -Wall -O0 -Wall -Werror -Iinc

all: boot kernel
	dd if=./bin/boot.bin >> ./bin/os.bin
	dd if=./bin/stage2.bin >> ./bin/os.bin
	dd if=./bin/kernel.bin >> ./bin/os.bin
	dd if=/dev/zero bs=512 count=100 >> ./bin/os.bin

boot: ./src/boot/boot.asm ./src/boot/stage2.asm
	nasm -f bin ./src/boot/boot.asm -o ./bin/boot.bin
	nasm -f bin ./src/boot/stage2.asm -o ./bin/stage2.bin

kernel: $(FILES)
	#x86_64-elf-ld -g $(FILES) -relocatable -o ./build/kernelfull.o
	x86_64-elf-ld -g -m elf_x86_64 -T ./src/kernel_link.ld $(FILES) -o ./build/kernelfull.elf
	x86_64-elf-objcopy -O binary ./build/kernelfull.elf ./bin/kernel.bin
	#x86_64-elf-gcc $(FLAGS) -m64 -T ./src/kernel_link.ld ./build/kernelfull.o -o ./bin/kernel.bin

./build/kernel.asm.o: ./src/kernel/kernel.asm
	nasm -f elf64 -g ./src/kernel/kernel.asm -o ./build/kernel.asm.o

./build/kernel.o: ./src/kernel/kernel.c
	x86_64-elf-gcc $(FLAGS) -m64 -c -std=gnu99 $(INCLUDE) ./src/kernel/kernel.c -o ./build/kernel.o

./build/memory.o: ./src/kernel/memory/memory.c
	x86_64-elf-gcc $(FLAGS) -m64 -c -std=gnu99 $(INCLUDE) ./src/kernel/memory/memory.c -o ./build/memory.o

./build/kheap.o: ./src/kernel/memory/kheap/kheap.c
	x86_64-elf-gcc $(FLAGS) -m64 -c -std=gnu99 $(INCLUDE) ./src/kernel/memory/kheap/kheap.c -o ./build/kheap.o

./build/gdt.o: ./src/kernel/gdt/gdt.c
	x86_64-elf-gcc $(FLAGS) -m64 -c -std=gnu99 $(INCLUDE) ./src/kernel/gdt/gdt.c -o ./build/gdt.o

./build/gdt.asm.o: ./src/kernel/gdt/gdt.asm
	nasm -f elf64 -g ./src/kernel/gdt/gdt.asm -o ./build/gdt.asm.o

./build/paging.o: ./src/kernel/memory/paging/paging.c
	x86_64-elf-gcc $(FLAGS) -m64 -c -std=gnu99 $(INCLUDE) ./src/kernel/memory/paging/paging.c -o ./build/paging.o

./build/paging.asm.o: ./src/kernel/memory/paging/paging.asm
	nasm -f elf64 -g ./src/kernel/memory/paging/paging.asm -o ./build/paging.asm.o

./build/terminal.o: ./src/kernel/terminal.c
	x86_64-elf-gcc $(FLAGS) -m64 -c -std=gnu99 $(INCLUDE) ./src/kernel/terminal.c -o ./build/terminal.o

./build/string.o: ./src/kernel/string/string.c
	x86_64-elf-gcc $(FLAGS) -m64 -c -std=gnu99 $(INCLUDE) ./src/kernel/string/string.c -o ./build/string.o

./build/io.asm.o: ./src/kernel/io/io.asm
	nasm -f elf64 -g ./src/kernel/io/io.asm -o ./build/io.asm.o

./build/idt.o: ./src/kernel/idt/idt.c
	x86_64-elf-gcc $(FLAGS) -m64 -c -std=gnu99 $(INCLUDE) ./src/kernel/idt/idt.c -o ./build/idt.o

./build/idt.asm.o: ./src/kernel/idt/idt.asm
	nasm -f elf64 -g ./src/kernel/idt/idt.asm -o ./build/idt.asm.o

./build/irqHandler.o: ./src/kernel/idt/irqHandler.c
	x86_64-elf-gcc $(FLAGS) -m64 -c -std=gnu99 $(INCLUDE) ./src/kernel/idt/irqHandler.c -o ./build/irqHandler.o

./build/exceptionHandler.o: ./src/kernel/idt/exceptionHandler.c
	x86_64-elf-gcc $(FLAGS) -m64 -c -std=gnu99 $(INCLUDE) ./src/kernel/idt/exceptionHandler.c -o ./build/exceptionHandler.o

clean:
	rm -r ./bin/*.bin
	rm -r ./build/*.o
	rm -r ./build/*.elf

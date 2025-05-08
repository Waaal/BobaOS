SHELL := /bin/bash

COMP_PATH = ${HOME}/opt/cross64/bin/
COMPILER = $(COMP_PATH)x86_64-elf-gcc
LINKER = $(COMP_PATH)x86_64-elf-ld
OBJCOPY = $(COMP_PATH)x86_64-elf-objcopy

FILES= ./build/kernel.asm.o ./build/kernel.o ./build/memory.o ./build/kheap.o ./build/kheapP.o ./build/kheapB.o ./build/gdt.o ./build/gdt.asm.o ./build/paging.o ./build/paging.asm.o ./build/terminal.o ./build/string.o ./build/io.asm.o ./build/idt.o ./build/idt.asm.o ./build/irqHandler.o ./build/exceptionHandler.o ./build/koal.o ./build/print.o ./build/pci.o ./build/disk.o ./build/diskDriver.o ./build/ataPioDriver.o
INCLUDE= -I ./src/kernel/
FLAGS= -g -ffreestanding -mcmodel=kernel -fno-pic -fno-pie -mno-red-zone -falign-jumps -falign-functions -falign-labels -falign-loops -fstrength-reduce -fomit-frame-pointer -Wno-unused-parameter -finline-functions -fno-builtin -Wno-cpp -nostdlib -nostartfiles -nodefaultlibs -Wall -O0 -Wall -Werror -Iinc

all: boot kernel
	dd if=./bin/boot.bin >> ./bin/os.bin
	dd if=./bin/fsinfo.bin >> ./bin/os.bin
	dd if=./bin/stage2.bin >> ./bin/os.bin
	dd if=./bin/kernel.bin >> ./bin/os.bin
	dd if=/dev/zero bs=512 count=125 >> ./bin/os.bin
	truncate -s 32M ./bin/os.bin
	echo -ne '\xF8\xFF\xFF\x0F\xFF\xFF\xFF\xFF\xFF\xFF\xFF\x0F' | dd of=./bin/os.bin bs=1 seek=102400 conv=notrunc
	echo -ne '\xF8\xFF\xFF\x0F\xFF\xFF\xFF\xFF\xFF\xFF\xFF\x0F' | dd of=./bin/os.bin bs=1 seek=364544 conv=notrunc

boot: ./src/boot/boot.asm ./src/boot/fsinfo.asm ./src/boot/stage2.asm
	nasm -f bin ./src/boot/boot.asm -o ./bin/boot.bin
	nasm -f bin ./src/boot/fsinfo.asm -o ./bin/fsinfo.bin
	nasm -f bin ./src/boot/stage2.asm -o ./bin/stage2.bin

kernel: $(FILES)
	#x86_64-elf-ld -g $(FILES) -relocatable -o ./build/kernelfull.o
	$(LINKER) -g -m elf_x86_64 -T ./src/kernel_link.ld $(FILES) -o ./build/kernelfull.elf
	$(OBJCOPY) -O binary ./build/kernelfull.elf ./bin/kernel.bin
	#x86_64-elf-gcc $(FLAGS) -m64 -T ./src/kernel_link.ld ./build/kernelfull.o -o ./bin/kernel.bin

./build/kernel.asm.o: ./src/kernel/kernel.asm
	nasm -f elf64 -g ./src/kernel/kernel.asm -o ./build/kernel.asm.o

./build/kernel.o: ./src/kernel/kernel.c
	$(COMPILER) $(FLAGS) -m64 -c -std=gnu99 $(INCLUDE) ./src/kernel/kernel.c -o ./build/kernel.o

./build/memory.o: ./src/kernel/memory/memory.c
	$(COMPILER) $(FLAGS) -m64 -c -std=gnu99 $(INCLUDE) ./src/kernel/memory/memory.c -o ./build/memory.o

./build/kheap.o: ./src/kernel/memory/kheap/kheap.c
	$(COMPILER) $(FLAGS) -m64 -c -std=gnu99 $(INCLUDE) ./src/kernel/memory/kheap/kheap.c -o ./build/kheap.o

./build/kheapB.o: ./src/kernel/memory/kheap/kheap_buddy.c
	$(COMPILER) $(FLAGS) -m64 -c -std=gnu99 $(INCLUDE) ./src/kernel/memory/kheap/kheap_buddy.c -o ./build/kheapB.o

./build/kheapP.o: ./src/kernel/memory/kheap/kheap_page.c
	$(COMPILER) $(FLAGS) -m64 -c -std=gnu99 $(INCLUDE) ./src/kernel/memory/kheap/kheap_page.c -o ./build/kheapP.o

./build/gdt.o: ./src/kernel/gdt/gdt.c
	$(COMPILER) $(FLAGS) -m64 -c -std=gnu99 $(INCLUDE) ./src/kernel/gdt/gdt.c -o ./build/gdt.o

./build/gdt.asm.o: ./src/kernel/gdt/gdt.asm
	nasm -f elf64 -g ./src/kernel/gdt/gdt.asm -o ./build/gdt.asm.o

./build/paging.o: ./src/kernel/memory/paging/paging.c
	$(COMPILER) $(FLAGS) -m64 -c -std=gnu99 $(INCLUDE) ./src/kernel/memory/paging/paging.c -o ./build/paging.o

./build/paging.asm.o: ./src/kernel/memory/paging/paging.asm
	nasm -f elf64 -g ./src/kernel/memory/paging/paging.asm -o ./build/paging.asm.o

./build/terminal.o: ./src/kernel/terminal.c
	$(COMPILER) $(FLAGS) -m64 -c -std=gnu99 $(INCLUDE) ./src/kernel/terminal.c -o ./build/terminal.o

./build/string.o: ./src/kernel/string/string.c
	$(COMPILER) $(FLAGS) -m64 -c -std=gnu99 $(INCLUDE) ./src/kernel/string/string.c -o ./build/string.o

./build/io.asm.o: ./src/kernel/io/io.asm
	nasm -f elf64 -g ./src/kernel/io/io.asm -o ./build/io.asm.o

./build/idt.o: ./src/kernel/idt/idt.c
	$(COMPILER) $(FLAGS) -m64 -c -std=gnu99 $(INCLUDE) ./src/kernel/idt/idt.c -o ./build/idt.o

./build/idt.asm.o: ./src/kernel/idt/idt.asm
	nasm -f elf64 -g ./src/kernel/idt/idt.asm -o ./build/idt.asm.o

./build/irqHandler.o: ./src/kernel/idt/irqHandler.c
	$(COMPILER) $(FLAGS) -m64 -c -std=gnu99 $(INCLUDE) ./src/kernel/idt/irqHandler.c -o ./build/irqHandler.o

./build/exceptionHandler.o: ./src/kernel/idt/exceptionHandler.c
	$(COMPILER) $(FLAGS) -m64 -c -std=gnu99 $(INCLUDE) ./src/kernel/idt/exceptionHandler.c -o ./build/exceptionHandler.o

./build/koal.o: ./src/kernel/koal/koal.c
	$(COMPILER) $(FLAGS) -m64 -c -std=gnu99 $(INCLUDE) ./src/kernel/koal/koal.c -o ./build/koal.o

./build/print.o: ./src/kernel/print.c
	$(COMPILER) $(FLAGS) -m64 -c -std=gnu99 $(INCLUDE) ./src/kernel/print.c -o ./build/print.o

./build/pci.o: ./src/kernel/hardware/pci/pci.c
	$(COMPILER) $(FLAGS) -m64 -c -std=gnu99 $(INCLUDE) ./src/kernel/hardware/pci/pci.c -o ./build/pci.o

./build/disk.o: ./src/kernel/disk/disk.c
	$(COMPILER) $(FLAGS) -m64 -c -std=gnu99 $(INCLUDE) ./src/kernel/disk/disk.c -o ./build/disk.o

./build/diskDriver.o: ./src/kernel/disk/diskDriver.c
	$(COMPILER) $(FLAGS) -m64 -c -std=gnu99 $(INCLUDE) ./src/kernel/disk/diskDriver.c -o ./build/diskDriver.o

./build/ataPioDriver.o: ./src/kernel/disk/driver/ataPio.c
	$(COMPILER) $(FLAGS) -m64 -c -std=gnu99 $(INCLUDE) ./src/kernel/disk/driver/ataPio.c -o ./build/ataPioDriver.o

clean:
	rm -r ./bin/*.bin
	rm -r ./build/*.o
	rm -r ./build/*.elf

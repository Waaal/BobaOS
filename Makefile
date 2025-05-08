SHELL := /bin/bash

COMP_PATH = ${HOME}/opt/cross64/bin/
COMPILER = $(COMP_PATH)x86_64-elf-gcc
LINKER = $(COMP_PATH)x86_64-elf-ld
OBJCOPY = $(COMP_PATH)x86_64-elf-objcopy

INCLUDE= -I ./src/kernel/
FLAGS= -g -ffreestanding -mcmodel=kernel -fno-pic -fno-pie -mno-red-zone -falign-jumps -falign-functions -falign-labels -falign-loops -fstrength-reduce -fomit-frame-pointer -Wno-unused-parameter -finline-functions -fno-builtin -Wno-cpp -nostdlib -nostartfiles -nodefaultlibs -Wall -O0 -Wall -Werror -Iinc

#This file NEEDS to be at the start of the kernel.bin file. Otherwise it wont work
KERNEL_ASM = src/kernel/kernel.asm
KERNEL_ASM_OBJ = build/kernel.asm.o

ASM_SRC := $(shell find src/kernel -name "*.asm" | grep -v "kernel.asm")
ASM_OBJS := $(patsubst src/kernel/%.asm, build/%.asm.o, $(ASM_SRC))

C_SRC := $(shell find src/kernel -name "*.c")
C_OBJS := $(patsubst src/kernel/%.c, build/%.o, $(C_SRC))

all: boot kernel
	@echo "Moving bootloader and kernel in os.bin file"
	@dd if=./bin/boot.bin >> ./bin/os.bin
	@dd if=./bin/fsinfo.bin >> ./bin/os.bin
	@dd if=./bin/stage2.bin >> ./bin/os.bin
	@dd if=./bin/kernel.bin >> ./bin/os.bin
	@dd if=/dev/zero bs=512 count=125 >> ./bin/os.bin
	@truncate -s 32M ./bin/os.bin
	@echo -ne '\xF8\xFF\xFF\x0F\xFF\xFF\xFF\xFF\xFF\xFF\xFF\x0F' | dd of=./bin/os.bin bs=1 seek=102400 conv=notrunc
	@echo -ne '\xF8\xFF\xFF\x0F\xFF\xFF\xFF\xFF\xFF\xFF\xFF\x0F' | dd of=./bin/os.bin bs=1 seek=364544 conv=notrunc

boot: ./src/boot/boot.asm ./src/boot/fsinfo.asm ./src/boot/stage2.asm
	nasm -f bin ./src/boot/boot.asm -o ./bin/boot.bin
	nasm -f bin ./src/boot/fsinfo.asm -o ./bin/fsinfo.bin
	nasm -f bin ./src/boot/stage2.asm -o ./bin/stage2.bin

kernel: $(KERNEL_ASM_OBJ) $(ASM_OBJS) $(C_OBJS)
	$(LINKER) -g -m elf_x86_64 -T ./src/kernel_link.ld $(KERNEL_ASM_OBJ) $(ASM_OBJS) $(C_OBJS) -o ./build/kernelfull.elf
	$(OBJCOPY) -O binary ./build/kernelfull.elf ./bin/kernel.bin

$(KERNEL_ASM_OBJ): $(KERNEL_ASM)
	nasm -f elf64 -g $< -o $@

build/%.asm.o: src/kernel/%.asm
	@mkdir -p $(dir $@)
	nasm -f elf64 -g $< -o $@

build/%.o: src/kernel/%.c
	@mkdir -p $(dir $@)
	$(COMPILER) -m64 $(FLAGS) $(INCLUDE) -c $< -o $@ 

clean:
	@echo "Clean bin and build directorys"
	@rm -r ./build/*
	@rm -r ./bin/*

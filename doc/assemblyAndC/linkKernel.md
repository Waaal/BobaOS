# Linking the Kernel
The Kernel is a special file, because it consists out of many files both C and asm. The linker needs to now the exact location where the kernel file is in memory, so it can calculate all the offsets right.

## Link small object files into one
The first step is to link all small object files into one big file.

This can be done very easily with this command
```
CrossCompilerName-ld -g -relocatable $(FILES) -o ./build/kernelfull.o
```

**$(FILES)** replace with paths to all object files.

**-g** is for debug symbols.

**-relocatable** Means that the output file is still a object file, that needs to be linked/compiled again. This mode suppresses many passes done for an executable or shared object output.

**-o** output file location and name.

## Create kernel binary
The final kernel binary can be created ith the following command
```
CrossCompilerName-gcc $(FLAGS) -T ./src/linker.ld ./build/kernelfull.o -o ./bin/kernel.bin
```

**$(FLAGS)** please see [CompileC Flags](compileC.md#FLAGS)

**-T** linker script

**-o** output file location and name.

This compiles the final kernel binary.

**Note:** The output format bin/elf is specified in the linker script.

### Linker script

```
ENTRY(_start)
OUTPUT_FORMAT(binary)
SECTIONS
{
    . = 1M;
    .text : ALIGN(4096)
    {
        *(.text)
    }

    .rodata : ALIGN(4096)
    {
        *(.rodata)
    }

    .data : ALIGN(4096)
    {
        *(.data)
    }

    .bss : ALIGN(4096)
    {
        *(COMMON)
        *(.bss)
    }

    .asm : ALIGN(4096)
    {
        *(.asm)
    }
}
```

**ENTRY(_start)** Specifies the main function. In this case it is *_start*

**OUTPUT_FORMAT(binary)** Specifies the output format. In this case *binary*

**SECTIONS** Specifies the order of the sections and some options. The order of the sections is defined by the order in the linker script. So in this case the .text section is the first, then the .rodata etc

**. = 1M** Specifies that this code gets loaded at address 0x100000 (1Mb).

**.text** Where our (C) code goes 

**.rodata** Where our constant variables goes

**.data** Where our normal variables goes

**.bss** Where our variables goes, thet dont have a value at compile time

**.asm** Our custom section. There goes our assembly code

**ALIGN(4096)** Tell the linker it should align every section to 4kb

### Important 

We created a custom section for our assembly, because assembly is not aligned by the nasm compiler. Our gcc c compiler aligns all the code as we specified with in [CompileC Alignment Flags](compileC.md#Alignmentflags). But assembly is not aligned by the compiler, so we place it in a seperate section and place this section at the end. So our not aligned assembly cannot put all of our C stuff out of alignment.

Also important is, that we need to have the .text section at the beginning. Because this kernel file is a binary file and doesnt have a header that specifies where our entry point is. So when we jump from our bootloader to the kernel, it jumps at the fist instruction of our binary file. So it is important, that the .text section is at the beginning and that our entry point is the fist thing in the binary file.

Every assembly file that needs to be in the text section needs to be manually aligned by us. So lets say our main kernel function is assembly, so our start lable is in assembly. This assembly then needs to jump to our c entry point. 
Because we said the start lable needs to be the first thing in our binary file we cannot put this kernel assemlby in the asm section. So we need to put it at the start. Because everything else in the .text section is aligned by the C compiler, we need to manually align our kernel assembly entry point.
``` assembly
times 512- ($ - $$) db 0 	; Aligns our kernel assembly.
```


*Note: The first thing in a binary file is specified by the order we give the files to the compiler*
```
CrossCompilerName-ld -g -relocatable ./build/file1.o ./build/file2.o ./build/file3.o -o ./build/kernelfull.o
```
*So file 1 will be the first file in our binary. (So if we need the kernel start point at the fist byte in our binary file, we need to put kernel.asm.o in the compiler first)*
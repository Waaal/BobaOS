# Compile Assembly
Assembly gets compiled into a object file or directly into a bin file.
A bin file is usefull for the bootloader.
A objectfile is usefull if we want to use our assembly to link into a bigger file.

To compile assembly nasm is a good choice.

### Compile a bin file
```
nasm -f bin ./src/boot/boot.asm -o ./bin/boot.bin
```

### Compile a elf (object) file
```
nasm -f elf -g ./src/kernel/kernel.asm -o ./build/kernel.asm.o
```

### Command line arguments explenation

**-f:** Specifies the output file format. (bin = binary, elf = object)

**-g:** Enable Debug information in the output file. For example to keep the symbol names so we can later use them in a debugger.

**-o:** Defines the output file location and name.

## Export/Import functions
If we want to use some of our assembly functions in C and call some C functions from assembly, the linker needs to find and link to this functions.

For this to work, we need to specify this in our assembly code. We need to refer to C functions and export our own functions, if they should be used in C.

### Export functions
``` assembly
global functionName		; We can make functions available for the linker with the global keyword

functionName:
	mov eax, 1
	ret
```
*Note: That C can use this global functions we obviously need a header file, that specifies to C, that this function exists*

### Import functions
``` assembly
extern functionName		; Tell the linker to search for this function in another file

example:
	mov eax, 1
	call functionName
```

### Function calls from C 
If we want to call a assembly function from our C code, we need to find out, where the arugments are.

## 32 bit 
In 32 bit the argumetns are pushed onto the stack, AFTER the return address for our ret instruction.

So the stack looks like this:
```
0x1b 	arg2
0x08	arg1
0x04	retAddress
```

## 64 bit
In 64-bit it is handled differently. The arguments arent on the stack anymore (at least the first 6) they are in registers.

The fist 6 argument are in registers and after that, they are pushed onto the stack.
```
rdi	arg1
rsi	arg2
rdx	arg3
rcx	arg4
r8	arg5
r9	arg6
```

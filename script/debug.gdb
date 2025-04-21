add-symbol-file build/kernelfull.elf 0x100000
target remote localhost:1234
set disassembly-flavor intel
b kmain
#b *0x1060c6
#layout asm
layout src
c

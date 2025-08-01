add-symbol-file build/kernelfull.elf 0x110000
target remote localhost:1234
set disassembly-flavor intel
b kmain
layout src
c

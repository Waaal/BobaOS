add-symbol-file ../build/kernelfull.elf
target remote localhost:1234
set disassembly-flavor intel
break kmain
c
layout src

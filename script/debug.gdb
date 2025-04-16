add-symbol-file /home/luke/BobaOS/build/kernelfull.elf
target remote localhost:1234
set disassembly-flavor intel
break kmain
c
layout src

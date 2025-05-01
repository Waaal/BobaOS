#/bin/bash

qemu-system-x86_64 -m 4G -s -S -device piix3-ide,id=ide -drive id=disk,file=./bin/os.bin,format=raw,if=none -device ide-hd,drive=disk,bus=ide.0 &
#qemu-system-x86_64 -m 4G -s -S -hda ./bin/os.bin &
gdb -x script/debug.gdb

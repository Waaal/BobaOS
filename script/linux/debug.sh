#/bin/bash

qemu-system-x86_64 -m 4G -s -S -hda ./build/os.bin &
gdb -x script/debug.gdb

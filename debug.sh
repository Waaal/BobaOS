#/bin/bash
qemu-system-x86_64 -s -S -hda ./bin/os.bin &
gdb -x script/debug.gdb

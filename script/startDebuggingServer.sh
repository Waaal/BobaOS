#!/bin/bash

# This file starts a debugging server for a IDE that can connect to this
# For debugging with pure GDB please use the debug.sh script
qemu-system-x86_64 -machine q35 -m 4G -drive id=disk,file=/home/luke/BobaOS/cmake-build-debug/os.bin,format=raw,if=none -device ahci,id=ahci -device ide-hd,drive=disk,bus=ahci.0 -S -gdb tcp::1234
#qemu-system-x86_64 -m 4G -hda /home/luke/BobaOS/cmake-build-debug/os.bin -S -gdb tcp::1234
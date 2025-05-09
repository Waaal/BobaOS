#!/bin/bash

# This file starts a debugging server for a IDE that can connect to this
# For debugging with pure GDB please use the debug.sh script
qemu-system-x86_64 -m 4G -hda /home/luke/BobaOS/bin/os.bin -S -gdb tcp::1234

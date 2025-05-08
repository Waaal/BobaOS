#!/bin/bash
qemu-system-x86_64 -m 4G -hda /home/luke/BobaOS/bin/os.bin -S -gdb tcp::1234
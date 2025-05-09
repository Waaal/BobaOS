#!/bin/bash

# Pls dont execute.
# This file is executed bu the makefile/cmake
echo -ne '\xF8\xFF\xFF\x0F\xFF\xFF\xFF\xFF\xFF\xFF\xFF\x0F' | dd of=/home/luke/BobaOS/build/os.bin bs=1 seek=102400 conv=notrunc
echo -ne '\xF8\xFF\xFF\x0F\xFF\xFF\xFF\xFF\xFF\xFF\xFF\x0F' | dd of=/home/luke/BobaOS/build/os.bin bs=1 seek=364544 conv=notrunc

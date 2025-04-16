#/bin/bash
export PREFIX="$HOME/opt/cross64"
export TARGET=x86_64-elf
export PATH="$PREFIX/bin:$PATH"
# make with x86_64-elf-gcc and the -m64 flag 64 bit files.

make clean
make

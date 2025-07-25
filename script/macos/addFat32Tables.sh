#!/bin/sh

# Pls dont execute.
# This file is executed bu the makefile/cmake

if [ $# == 1 ]; then
  echo -ne '\xF8\xFF\xFF\x0F\xFF\xFF\xFF\xFF\xFF\xFF\xFF\x0F' | dd of=$1 bs=1 seek=153600 conv=notrunc
  echo -ne '\xF8\xFF\xFF\x0F\xFF\xFF\xFF\xFF\xFF\xFF\xFF\x0F' | dd of=$1 bs=1 seek=415744 conv=notrunc
else
  echo "Missing arguments"
fi

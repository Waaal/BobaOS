#!/bin/bash

ELF="$1"

if [ -z "$ELF" ]; then
	echo "Usage: $0 <path-to-elf-file>"
	exit 1
fi

echo "Checking alignment of .text and .text.asm section in $ELF"
echo ""

readelf -S "$ELF" | grep text | grep -v readelf

echo ""
echo "Please verify that these sections are aligned to 4096 (0x1000)"

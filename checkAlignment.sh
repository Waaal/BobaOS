#!/bin/bash

ELF="$1"

if [ -z "$ELF" ]; then
    echo "Usage: $0 <path-to-elf-file>"
    exit 1
fi

echo "🔍 Checking alignment of sections in $ELF"
echo

readelf -S "$ELF" | awk '
BEGIN { print "Section Name         Addr        Align" }
/^\s*\[[0-9]+\]/ {
    name = $2
    addr = $5
    align = $NF
    printf "%-20s 0x%-8s %s\n", name, addr, align
}
'


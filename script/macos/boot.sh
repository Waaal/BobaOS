qemu-system-x86_64 -m 4G -drive id=disk,file=/home/luke/BobaOS/build/os.bin,format=raw,if=none -device ahci,id=ahci -device ide-hd,drive=disk,bus=ahci.0

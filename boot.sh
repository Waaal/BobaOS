qemu-system-x86_64  -m 4G -device piix3-ide,id=ide -drive id=disk,file=./bin/os.bin,format=raw,if=none -device ide-hd,drive=disk,bus=ide.0 

# BobaOS
Boba

See [Docs](https://github.com/Waaal/BobaOS/tree/Docs/doc/)

## How to compile?
To compile you need **makefile** and the **x86_64-elf** cross compiler installed/compiled.

Use the **build.sh** script to build the project.

## How to run?
To run the project you need **Qemu** installed.

Use the **boot.sh** script to boot up the project with Qemu.

## How to debug?
To debug the kernel you need **gdb** installed.

Use the **debug.sh** command to start the debugging.
The script will launch qemu, load the objectfiles and break at the kernel entry (kernel.c:kmain) 

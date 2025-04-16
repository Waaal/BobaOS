# 🧋 BobaOS

Welcome to **BobaOS** – Boba

---

## 🔧 What is BobaOS?

BobaOS is a minimal x86-based 64 bit operating system focused on:

- A stable kernel
- Memory management
- Basic multitasking
- Simple filesystem structures
- Eventually a userland with a custom shell

---

## 🗺️ Development Roadmap

BobaOS is being developed in versioned stages:

| Version | Name             | Focus                                           |
|---------|------------------|--------------------------------------------------|
| v0.1    | 🧋 **Milk Tea**     | Bootloader, kernel base, memory init, paging, basic interrupt handling |
| v0.2    | 🧱 **Tapioca Core** | early kernel infrastructure, Virtual file system layer, Fat32 Filesystem support |
| v0.3    | ⚙️ **Boiling Point** | Tasks and Process base, syscall base, Userland  |

📌 Check out the full dev plan on the [GitHub Project Board](https://github.com/users/Waaal/projects/1/views/1)

---

## 🚀 Getting Started

> ⚠️ BobaOS is not ready for any task (or even proper booting yet), but feel free to explore the code and follow along.

### Requirements

- NASM
- x86_64-elf cross-compiler
- make
- QEMU
- GDB

### Building
There is a building script provided - just call **build.sh**.

*This script requires that you have the cross compiler under the path /home/USERNAME/opt/cross64*

```bash
export PREFIX="$HOME/opt/cross64"
export TARGET=x86_64-elf
export PATH="$PREFIX/bin:$PATH"

make clean
make
```

### Running (with QEMU)
There is a boot script provided - just call **boot.sh**.
```bash
qemu-system-x86_64  -hda ./bin/os.bin
```

### Debugging (with QEMU and GDB)
There is a debug script and a gdb script provided - just call **debug.sh**.

The debug script will load the objectfiles and will break at the kernel entry.

```bash
qemu-system-x86_64 -s -S -hda ./bin/os.bin &
gdb -x script/debug.gdb
```

---

## 📦 Releases

Each version milestone will be tagged and released here once stable.  
Stay tuned for `v0.1 – Milk Tea`.

---

## 🤝 Contributions

Right now this is a solo learning project, but feel free to open issues, share ideas, or fork and build your own flavor.

---


## 📜 License

This project is licensed under the MIT License.

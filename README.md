# 🧋 BobaOS

Welcome to **BobaOS** – Boba

---

## 🔧 What is BobaOS?

BobaOS is a x86-based 64 bit operating system.
Current features include:

- ATA PIO disk driver
- AHCI SATA disk driver
- FAT32 read and write support
- Working with PCI devices 
- Kernel heap with page block or buddy algorithm
- Virtual memory remapping 

---

## 🗺️ Development Roadmap

BobaOS is being developed in versioned stages:

| Version | Name             | Focus                                           |
|---------|------------------|--------------------------------------------------|
| v0.1    | 🧋  **Milk Tea**     | Bootloader, kernel base, memory init, paging, basic interrupt handling |
| v0.2    | 🧱 **Tapioca Core** | early kernel infrastructure, Virtual file system layer, Fat32 Filesystem support |
| v0.3    | 🍒 **Lychee Drift** | Tasks and Process base, syscall base, Userland, CPU/Core info |
| v0.4   | 🍬 **Brown Sugar Rush** | Multicore preparation, Virtual Keyboard Layer , Early Scheduler and dispatcher |

📌 Check out the full dev plan on the [GitHub Project Board](https://github.com/users/Waaal/projects/1/views/1)

---

## 🚀 Getting Started

> ⚠️ BobaOS is not ready for any real task, but feel free to explore the code and follow along.

### Requirements

- Linux as development environment
- NASM
- x86_64-elf cross-compiler
- CMAKE
- QEMU
- GDB

### Building, Booting & Debugging
The buildsystem is CMAKE.
```bash
cmake -B build -S .                 # Use on first time init
cmake --build build                 # Use to build the project
cmake --build build --target clean  # Also always use clean before building again
```


There is a boot and debug script provided for booting and debugging with qemu and gdb
```bash
boot.sh     # Boot the project with qemu
debug.sh    # Debug the project with qemu and gdb
```


The buildscript requires that you have the cross compiler at /home/USERNAME/opt/cross64


The debugging script will launch gdb and qemu for you and break at the kernel entry


#### Debug Assembly
Currently debugging assembly code in the kernel is a pain in the...


We placed our assembly code from the kernel in its own sesction (.text.asm) and the linker will put it as last section, so that our not aligned assembly (NASM cant really align it) cannot put anything aligned out of alignment.


But because it is a custom section, GDB wont let you set a breakpoint there.
So in order to debug assembly you have to:
```bash
info address ASSEMLY_LABEL
layout asm #Go into layout assembly before jumping to it, otherwise GDB will bug out
break *ADDRESS_ASSEMBLY_LABEL
```

---

## 📂 Folder structure
- assets/
- script/
- src/

**assets:** The files that get copied to the FAT32 system of BobaOS.


**script:** Scripts for various tasks and tools.


**src:** The source code of the project.

---

## 📦 Releases

[v0.1 - Milk Tea](https://github.com/Waaal/BobaOS/releases/tag/V0.1)
[v0.2 - Tapioca Core](https://github.com/Waaal/BobaOS/releases/tag/V0.2)

---

## 🤝 Contributions

Right now this is a learning by doing project, but feel free to open issues, share ideas, or fork and build your own flavor.

---


## 📜 License

This project is licensed under the MIT License.

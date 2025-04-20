# 🧋 BobaOS

Welcome to **BobaOS** – Boba

---

## 🔧 What is BobaOS?

BobaOS is a minimal x86-based 64 bit operating system focused on:

- Add a description as soon as it has features...

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

> ⚠️ BobaOS is not ready for any task (or even proper booting yet), but feel free to explore the code and follow along.

### Requirements

- Linux as development environment
- NASM
- x86_64-elf cross-compiler
- make
- QEMU
- GDB

### Building, Booting & Debugging
There is a script provided for building, running and debugging


```bash
build.sh    # Build the project with makefile and x86_64 elf cross-compiler
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

## 📦 Releases

Each version milestone will be tagged and released here once stable.  
Stay tuned for `v0.1 – Milk Tea`.

---

## 🤝 Contributions

Right now this is a learning project, but feel free to open issues, share ideas, or fork and build your own flavor.

---


## 📜 License

This project is licensed under the MIT License.

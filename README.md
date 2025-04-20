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

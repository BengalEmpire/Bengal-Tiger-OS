# Bengal Tiger OS v1.0.0 ("Royal Bengal")

[![Build, Test & Release](https://github.com/BengalEmpire/Bengal-Tiger-OS/actions/workflows/build-and-release.yml/badge.svg)](https://github.com/BengalEmpire/Bengal-Tiger-OS/actions/workflows/build-and-release.yml)
[![License: MIT](https://img.shields.io/badge/License-MIT-orange.svg)](licence)
[![Architecture: x86 32-bit](https://img.shields.io/badge/Arch-i386%20(32--bit)-blue.svg)](#architecture)

**Bengal Tiger OS** is a real-world usable 32-bit x86 operating system written from scratch in freestanding C and x86 Assembly. It boots via **GRUB Multiboot1** on QEMU, VirtualBox, VMware, and bare-metal x86 hardware, providing a high-resolution VBE graphical console & desktop workspace, preemptive multitasking, persistent FAT12/FAT16 storage (with automatic Live RAMDisk fallback on ISO boot), a built-in full-screen text editor, shell scripting, and a live TCP/IPv4 network stack (`DHCP` + `ARP` + `ICMP ping`).

---

## Key Features

- **Boot & CPU Foundation**:
  - Strict Multiboot1 ELF32 compliance (`0x1BADB002`, flags `0x00000007`), 32 KB 16-byte-aligned bootstrap stack, non-destructive A20 gate enable, CPUID vendor/brand/feature detection, x87 FPU + SSE initialization, custom GDT, 256-entry IDT, remapped 8259A PIC (with spurious IRQ7/IRQ15 guards), and `INT 0x80` system calls.
- **Memory & Preemptive Multitasking**:
  - BIOS E820-aware Physical Memory Manager (PMM), 32 MB identity-mapped virtual paging with dynamic uncached MMIO page tables, interrupt-safe expandable 16 MB kernel heap (`kmalloc`, `kzalloc`, `kmalloc_aligned`, `krealloc`, `kfree`), and a 100 Hz round-robin preemptive task scheduler (`spawn`, `ps`, `top`, `kill`).
- **Unified Console & Graphical Desktop (`gui`)**:
  - Simultaneous rendering across **24/32-bpp VBE Linear Framebuffer** (`1024x768x32` default with live UTC RTC status bar), **VGA 80x25 Text Mode** (with hardware CRT cursor), and **COM1 115200 8N1 Serial Console** (including VT100/ANSI arrow/navigation keys).
  - Interactive **Graphical Desktop (`gui` / `F2`)** with PS/2 mouse cursor tracking, live CPU/Heap/Task monitor, storage explorer, and one-click gateway ping.
- **Storage, Live RAMDisk & Full-Screen Editor (`edit`)**:
  - 4-slot Primary/Secondary ATA/ATAPI PIO driver + automatic **512 KB Live FAT12 RAMDisk** when booting from ISO/USB without a formatted disk.
  - Full **FAT12 / FAT16** driver supporting Superfloppy and MBR-partitioned disks, `mkfs` volume formatting, multi-cluster read/write/append/copy/rename/delete, shell scripts (`sh`), and an interactive full-screen text editor (`edit`).
- **Real TCP/IPv4 Networking**:
  - Dual PCI Ethernet drivers: **Intel PRO/1000 (`E1000`: 82540EM/82545EM/82574L)** (default in QEMU, VirtualBox, VMware) and **Realtek `RTL8139`**.
  - Ethernet II, ARP cache & resolution, IPv4 subnet routing, UDP, automatic **4-way DHCPv4 DORA client (`dhcp`)**, and bidirectional **ICMP Echo Request/Reply (`ping`)**.

---

## Quick Start

### 1. Download Ready-to-Boot ISO
Download **`bengaltiger.iso`** directly from the [**Latest GitHub Release**](https://github.com/BengalEmpire/Bengal-Tiger-OS/releases/latest) and boot it in:
- **QEMU**:
  ```bash
  qemu-system-i386 -cdrom bengaltiger.iso -m 64M -netdev user,id=n0 -device e1000,netdev=n0 -serial stdio
  ```
- **QEMU with Persistent Hard Disk**:
  ```bash
  dd if=/dev/zero of=disk.img bs=1M count=32
  qemu-system-i386 -cdrom bengaltiger.iso -hda disk.img -boot d -m 64M -netdev user,id=n0 -device e1000,netdev=n0
  # Inside Bengal Tiger OS, run `mkfs` once to format disk.img as persistent FAT16!
  ```
- **VirtualBox / VMware**: Create a 32-bit *Other / Unknown* VM with 64+ MB RAM, Intel PRO/1000 MT Desktop NAT networking, attach `bengaltiger.iso` to the optical drive, and boot.

### 2. Build & Verify from Source (Ubuntu / Debian / WSL)
```bash
sudo apt-get update
sudo apt-get install -y gcc-multilib binutils grub-pc-bin grub-common xorriso mtools qemu-system-x86 python3 make

git clone https://github.com/BengalEmpire/Bengal-Tiger-OS.git
cd Bengal-Tiger-OS

make iso          # Compile 32-bit kernel and package bootable bengaltiger.iso
make test         # Run ELF/Multiboot1 verifier + ASan/UBSan host unit tests
make test-qemu    # Run automated 2-boot QEMU E2E test (DHCP, Ping, FAT16 persistence)
make run          # Launch interactive QEMU session
```

---

## Shell Command Reference

| Category | Commands |
| :--- | :--- |
| **System & Info** | `help`, `info` (`neofetch`), `ver`, `cpu`, `date`, `uptime`, `dmesg`, `user [name]`, `history`, `echo <txt> [> file]`, `calc <expr>`, `beep [hz] [ms]`, `gui` (`F2`), `clear`, `reboot`, `shutdown` (`poweroff`) |
| **Memory & Tasks** | `mem` (`free`), `ps`, `top`, `spawn [count]`, `kill <pid>` |
| **Storage & FAT FS** | `ls` (`dir`), `cat <file>`, `edit <file>`, `write <file> <txt>`, `append <file> <txt>`, `cp <src> <dst>`, `mv <old> <new>`, `rm <file>`, `touch <file>`, `stat <file>`, `hexdump <file>`, `sh <script.sh>`, `df`, `disk`, `mkfs [label]` |
| **PCI & Networking** | `lspci`, `ifconfig [ip mask gw]`, `dhcp`, `ping <ip> [count]`, `arp`, `netstat` |

Line editing supports **Left/Right/Home/End/Delete**, **Up/Down command history**, **`!N` history replay**, **`Ctrl+C` / `Ctrl+L`**, and **Tab completion** for both commands and filenames.

---

## Documentation

- [System Architecture](docs/ARCHITECTURE.md)
- [Build & Testing Guide](docs/BUILD.md)
- [Kernel Subsystem API](docs/API.md)
- [Project Roadmap](docs/ROADMAP.md)
- [Changelog](CHANGELOG.md)

 > Want to test the OS? [Checkout docs](https://github.com/BengalEmpire/Bengal-Tiger-OS/blob/main/docs/TESTING_GUIDE.md)

## License

Released under the [MIT License](licence). Copyright (c) BengalEmpire.

# Bengal Tiger OS

```
██████╗ ███████╗███╗   ██╗ ██████╗  █████╗ ██╗            
██╔══██╗██╔════╝████╗  ██║██╔════╝ ██╔══██╗██║            
██████╔╝█████╗  ██╔██╗ ██║██║  ███╗███████║██║            
██╔══██╗██╔══╝  ██║╚██╗██║██║   ██║██╔══██║██║            
██████╔╝███████╗██║ ╚████║╚██████╔╝██║  ██║███████╗       
╚═════╝ ╚══════╝╚═╝  ╚═══╝ ╚═════╝ ╚═╝  ╚═╝╚══════╝       
                                                          
████████╗██╗ ██████╗ ███████╗██████╗     ██████╗ ███████╗
╚══██╔══╝██║██╔════╝ ██╔════╝██╔══██╗    ██╔═══██╗██╔════╝
   ██║   ██║██║  ███╗█████╗  ██████╔╝    ██║   ██║███████╗
   ██║   ██║██║   ██║██╔══╝  ██╔══██╗    ██║   ██║╚════██║
   ██║   ██║╚██████╔╝███████╗██║  ██║    ╚██████╔╝███████║
   ╚═╝   ╚═╝ ╚═════╝ ╚══════╝╚═╝  ╚═╝     ╚═════╝ ╚══════╝
```

**Bengal Tiger OS** is a 32-bit x86 (i386) open-source operating system written in ANSI C and x86 Assembly. Designed with modular architecture, hardware abstraction layers, VBE graphics, FAT filesystem persistence, Realtek network drivers, and a preemptive task scheduler, Bengal Tiger OS serves as a production-quality, educational, and real-world-ready operating system.

---

## ✨ New in Version 0.6.0 — "Real-World OS Kernel Release"

### 🔄 Preemptive Multitasking Task Scheduler
- **Round-Robin Task Scheduler**: Preemptive task switching executing on timer interrupts (IRQ0).
- **Task Management**: Dynamic thread creation (`task_create`), voluntary task yields (`task_exit`), and process termination (`task_kill`).
- **Context Preservation**: Register frame stack switching (`struct regs*`) in hardware ISR stubs.

### 🌐 Realtek RTL8139 Hardware Network Driver
- **PCI Discovery**: Automatic scanning and initialization for Realtek 8139 NICs (Vendor `0x10EC`, Device `0x8139`).
- **Circular Ring Buffer**: 8KB physical RX ring buffer management with CAPR updating.
- **TX Descriptors**: 4-descriptor round-robin hardware transmit queues.
- **Hardware MAC Address**: Real-time reading of EEPROM/registers and Ethernet frame construction.

### 💾 FAT Filesystem Write & Cluster Chaining
- **File Creation & Modification**: Supports creating new directory entries and writing file contents (`fat_save_file`).
- **Multi-Cluster Chaining**: Dynamically allocates clusters and updates FAT12/FAT16 tables.
- **Root Directory Allocation**: Finds empty/deleted directory slots (`0x00`/`0xE5`) for new files.

### 🐚 New Shell Commands
| Command | Description |
|---------|-------------|
| `ps` | List running kernel tasks, PIDs, and process states |
| `kill <pid>` | Terminate a task by its Process ID (PID) |
| `ifconfig` | Display network interfaces, I/O base, MAC address, and TX/RX packet statistics |
| `ping <ip>` | Transmit test Ethernet frames over the RTL8139 network interface |
| `touch <file>` | Create a new empty file in the FAT root directory |
| `write <file> <text>` | Write text into a file on the FAT volume |

---

## 🏗️ Architecture

```
┌─────────────────────────────────────────────────────────────┐
│                      USER SHELL                             │
│  Commands: help, neofetch, pci, mem, uptime, date, disk,    │
│           cpu, vbe, mouse, ps, kill, ifconfig, ping,        │
│           touch, write, cat, ls, echo, clear, reboot,...    │
├─────────────────────────────────────────────────────────────┤
│                    KERNEL SUBSYSTEMS                          │
│  ┌──────────┐ ┌──────────┐ ┌──────────┐ ┌──────────┐       │
│  │  RTC     │ │  Serial  │ │   Disk   │ │   FAT    │       │
│  │ (CMOS)   │ │ (UART)   │ │(ATA PIO) │ │ (R/W)    │       │
│  └──────────┘ └──────────┘ └──────────┘ └──────────┘       │
│  ┌──────────┐ ┌──────────┐ ┌──────────┐ ┌──────────┐       │
│  │  Timer   │ │ Keyboard │ │   Heap   │ │Scheduler │       │
│  │  (PIT)   │ │  (PS/2)  │ │ Allocator│ │(Preempt) │       │
│  └──────────┘ └──────────┘ └──────────┘ └──────────┘       │
├─────────────────────────────────────────────────────────────┤
│                    GRAPHICS & INPUT                          │
│  ┌────────────────────┐  ┌────────────────────┐             │
│  │  VBE Framebuffer   │  │   PS/2 Mouse Driver│             │
│  │ Pixel/Shape/Text   │  │ Hardware Cursor    │             │
│  └────────────────────┘  └────────────────────┘             │
├─────────────────────────────────────────────────────────────┤
│               NETWORK DRIVER & BUS LAYER                      │
│  ┌────────────────────┐  ┌────────────────────┐             │
│  │ Realtek RTL8139    │  │     PCI Bus        │             │
│  │ RX Ring / TX Descs │  │ Enum & Scanner     │             │
│  └────────────────────┘  └────────────────────┘             │
├─────────────────────────────────────────────────────────────┤
│                   CPU / SYSTEM LAYER                         │
│  ┌──────────┐ ┌──────────┐ ┌──────────┐ ┌──────────┐       │
│  │   GDT    │ │  CPUID   │ │  A20     │ │  FPU/SSE │       │
│  │   Own    │ │ Features │ │  Gate    │ │   Init   │       │
│  └──────────┘ └──────────┘ └──────────┘ └──────────┘       │
├─────────────────────────────────────────────────────────────┤
│                  HARDWARE ABSTRACTION                         │
│  ┌──────────┐ ┌──────────┐ ┌──────────┐ ┌──────────┐       │
│  │   IDT    │ │   PIC    │ │  Paging  │ │ Context  │       │
│  │  (256)   │ │  (8259)  │ │ (Dynamic)│ │ Switch   │       │
│  └──────────┘ └──────────┘ └──────────┘ └──────────┘       │
├─────────────────────────────────────────────────────────────┤
│               BOOT (GRUB/Multiboot + Stack Init)             │
└─────────────────────────────────────────────────────────────┘
```

---

## 🛠️ Build & Run Instructions

### Prerequisites (Debian/Ubuntu)
```bash
sudo apt update
sudo apt install build-essential gcc-multilib grub-pc-bin grub-common xorriso mtools qemu-system-x86
```

### Build Kernel Binary
```bash
make build
make build/kernel.bin
```

### Create Bootable ISO & Run in QEMU
```bash
make all
make run
```

---

## 🗺️ Roadmap: Path to a Professional Real-World OS

To further evolve Bengal Tiger OS into an enterprise-grade, general-purpose production operating system, the development roadmap encompasses:

1. **Virtual File System (VFS) Abstraction Layer**:
   - POSIX-compliant file descriptor tables (`open`, `read`, `write`, `close`).
   - Mounting architecture supporting ext2, FAT32, and ramfs filesystems.

2. **Network Protocol Stack (TCP/IP)**:
   - ARP resolution, IPv4 routing, ICMP protocol engine.
   - UDP and TCP socket layers with BSD Sockets API (`socket`, `bind`, `connect`, `send`, `recv`).

3. **User-Mode Execution & Privilege Separation (Ring 0 vs Ring 3)**:
   - Dynamic ELF executable binary loader (`/bin/sh`, `/bin/ls`).
   - Task State Segment (TSS) setup and user-mode page table isolation (`PAGE_USER`).
   - Software interrupt `INT 0x80` / `SYSENTER` system call ABI.

4. **Symmetric Multiprocessing (SMP)**:
   - Advanced Programmable Interrupt Controller (APIC / IOAPIC) driver.
   - Inter-Processor Interrupts (IPI) and multi-core CPU initialization via ACPI.

5. **Enhanced USB Host Controller & Storage Drivers**:
   - EHCI (USB 2.0) and xHCI (USB 3.0) host controller interface drivers.
   - USB Mass Storage class driver for USB flash drive booting and persistence.

---

## 📞 Contact & License

Created with 💛 by BengalEmpire

*Roar like a tiger, code like a pro! 🐯*

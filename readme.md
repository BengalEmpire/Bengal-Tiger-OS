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

**Bengal Tiger OS** is a 32-bit hobby operating system written in C and x86 assembly, engineered for booting on real x86 hardware. It boots via GRUB with a full Multiboot-compliant kernel, features a colorful terminal shell, hardware drivers, VESA framebuffer graphics, and a robust initialization pipeline. This OS is designed for learning OS development and provides a production-quality foundation.

---

## ✨ New in Version 0.5.0 — "Graphics Mode"

### 🎨 VBE Framebuffer Graphics
- **VESA Linear Framebuffer** — High-resolution graphics via GRUB's `gfxpayload`
- **Multiple Resolutions** — 1024×768×32, 800×600×32, 1280×1024×32
- **Pixel Drawing** — `vbe_putpixel()`, `vbe_fill_rect()`, `vbe_draw_rect()`, lines
- **Text Rendering** — 8×16 bitmap font with word wrap and newline support
- **Screen Management** — Clear, scroll, color fills
- **16 VGA Colors** — Pre-mapped to 32-bit RGBA

### 🐚 New Shell Command
| Command | Description |
|---------|-------------|
| `vbe` | Show framebuffer info + draw a demo pattern (color bars, border, text) |

### 📋 Updated Features
- `neofetch` now shows graphics resolution when VBE is active
- 5 GRUB boot entries: text mode + 3 graphics modes + debug

### 📁 New Files
| File | Description |
|------|-------------|
| `kernel/vbe.h` | VBE framebuffer types and drawing API |
| `kernel/vbe.c` | VBE init, pixel/rect/text rendering, paging mapping |
| `kernel/font.h` | 8×16 bitmap font header |
| `kernel/font.c` | 8×16 bitmap font data (95 ASCII characters) |

---

## 🏗️ Architecture

```
┌─────────────────────────────────────────────────────────────┐
│                      USER SHELL                               │
│  Commands: help, neofetch, pci, mem, uptime, date, disk,    │
│           cpu, vbe, echo, clear, color, history, reboot,...  │
├─────────────────────────────────────────────────────────────┤
│                    KERNEL SUBSYSTEMS                          │
│  ┌──────────┐ ┌──────────┐ ┌──────────┐ ┌──────────┐       │
│  │  RTC     │ │  Serial  │ │   Disk   │ │   FAT    │       │
│  │ (CMOS)   │ │ (UART)   │ │(ATA PIO) │ │Filesystem│       │
│  └──────────┘ └──────────┘ └──────────┘ └──────────┘       │
│  ┌──────────┐ ┌──────────┐ ┌──────────┐ ┌──────────┐       │
│  │  Timer   │ │ Keyboard │ │   Heap   │ │Scheduler │       │
│  │  (PIT)   │ │  (PS/2)  │ │ Allocator│ │  (stub)  │       │
│  └──────────┘ └──────────┘ └──────────┘ └──────────┘       │
├─────────────────────────────────────────────────────────────┤
│                    GRAPHICS LAYER                             │
│  ┌────────────────────┐  ┌────────────────────┐             │
│  │  VBE Framebuffer   │  │  8x16 Bitmap Font  │             │
│  │ Pixel/Shape/Text   │  │  95 ASCII glyphs   │             │
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
│  │   IDT    │ │   PIC    │ │  Paging  │ │   PCI    │       │
│  │  (256)   │ │  (8259)  │ │ (Dynamic)│ │  Scanner │       │
│  └──────────┘ └──────────┘ └──────────┘ └──────────┘       │
├─────────────────────────────────────────────────────────────┤
│               BOOT (GRUB/Multiboot + Stack Init)             │
└─────────────────────────────────────────────────────────────┘
```

---

## 📁 Project Structure

```
Bengal-Tiger-OS/
├── boot/
│   └── boot.s              # Multiboot entry + stack init
├── grub/
│   └── grub.cfg            # GRUB bootloader config (with graphics mode entries)
├── kernel/
│   ├── main.c              # Kernel entry point (7-phase init)
│   ├── common.c/h          # Utility functions, types, I/O ports
│   ├── multiboot.h         # Multiboot info & E820 memory map
│   ├── gdt.c/h             # Global Descriptor Table (own GDT)
│   ├── cpu.c/h             # CPUID, A20 gate, FPU/SSE init
│   ├── idt.c/h             # Interrupt Descriptor Table
│   ├── isr.s               # Interrupt service routines (asm)
│   ├── isr.c               # Interrupt handlers (C)
│   ├── pic.c/h             # 8259 PIC controller
│   ├── paging.c/h          # Virtual memory (dynamic allocation)
│   ├── heap.c/h            # Dynamic memory allocator
│   ├── timer.c/h           # PIT timer driver
│   ├── keyboard.c/h        # PS/2 keyboard driver
│   ├── rtc.c/h             # CMOS Real-Time Clock driver
│   ├── serial.c/h          # 16550 UART serial driver
│   ├── disk.c/h            # Enhanced ATA/ATAPI driver
│   ├── fat.c/h             # FAT filesystem (stub)
│   ├── vbe.c/h             # VBE framebuffer driver
│   ├── font.c/h            # 8x16 bitmap font data
│   ├── pci.c/h             # PCI bus scanner
│   ├── nic.c/h             # Network driver (stub)
│   ├── scheduler.c/h       # Task scheduler (stub)
│   ├── shell.c/h           # Command-line interface
│   └── panic.c/h           # Kernel panic handler
├── linker.ld               # Linker script
├── makefile                # Build system (24 object files)
└── docs/                   # Documentation
```

---

## 🛠️ Build Instructions

- Check the [BUILD.md](docs/BUILD.md) file for detailed build instructions.

## 📖 Using Bengal Tiger OS

### Boot Process
1. GRUB menu appears (5 second timeout) — select **text mode** or a **graphics mode**
2. Boot log scrolls: A20 → GDT → CPU → PIC → IDT → PMM → Paging → Heap → VBE → Serial → Timer → Keyboard → RTC → ATA → FAT → Scheduler → PCI → NIC → Interrupts
3. Bengal Tiger logo animation plays
4. Enter your username (first boot) or shell prompt appears
5. Shell prompt: `yourname@bengal-tiger:~$`

### Available Commands

#### File Commands
| Command | Description |
|---------|-------------|
| `ls` | List files |
| `cat <file>` | Display file contents |

#### System Commands
| Command | Description |
|---------|-------------|
| `info` | Show OS version |
| `neofetch` | Display system info with ASCII art |
| `uptime` | Show system uptime |
| `mem` | Show memory usage |
| `pci` | List PCI devices |
| `date` | Show real date/time (RTC) |
| `disk` | Show ATA drive information |
| `cpu` | Show CPU vendor, brand, features |
| `vbe` | Show VBE framebuffer info + draw demo pattern |

#### Shell Commands
| Command | Description |
|---------|-------------|
| `help` | Show all commands |
| `echo <msg>` | Print message |
| `clear` | Clear screen |
| `color <0-15>` | Set text color |
| `history` | Show command history |

#### Control Commands
| Command | Description |
|---------|-------------|
| `reboot` | Reboot system |
| `shutdown` | Halt system |
| `exit` | Alias for shutdown |

### Keyboard Shortcuts
| Key | Function |
|-----|----------|
| `↑` / `↓` | Navigate command history |
| `←` / `→` | Move cursor in line |
| `Home` | Jump to line start |
| `End` | Jump to line end |
| `Backspace` | Delete character before cursor |
| `Delete` | Delete character at cursor |
| `Shift+Letter` | Uppercase letter |
| `Caps Lock` | Toggle uppercase |

### Debug via Serial
Connect a serial terminal (e.g., `picocom`, `screen`, `PuTTY`) to COM1 at 115200 baud to see kernel boot logs and debug output in real time.

---

## 🎨 VGA Colors

Use `color <n>` to change text color:

| # | Color | # | Color |
|---|-------|---|-------|
| 0 | Black | 8 | Dark Grey |
| 1 | Blue | 9 | Light Blue |
| 2 | Green | 10 | Light Green |
| 3 | Cyan | 11 | Light Cyan |
| 4 | Red | 12 | Light Red |
| 5 | Magenta | 13 | Light Magenta |
| 6 | Brown | 14 | Yellow |
| 7 | Light Grey | 15 | White |

---

## 📞 Contact

Created with 💛 by BengalEmpire

*Roar like a tiger, code like a pro! 🐯*

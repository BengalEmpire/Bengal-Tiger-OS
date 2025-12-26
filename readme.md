# Bengal Tiger OS

 ```
██████╗ ███████╗███╗   ██╗ ██████╗  █████╗ ██╗            
██╔══██╗██╔════╝████╗  ██║██╔════╝ ██╔══██╗██║            
██████╔╝█████╗  ██╔██╗ ██║██║  ███╗███████║██║            
██╔══██╗██╔══╝  ██║╚██╗██║██║   ██║██╔══██║██║            
██████╔╝███████╗██║ ╚████║╚██████╔╝██║  ██║███████╗       
╚═════╝ ╚══════╝╚═╝  ╚═══╝ ╚═════╝ ╚═╝  ╚═╝╚══════╝       
                                                          
████████╗██╗ ██████╗ ███████╗██████╗      ██████╗ ███████╗
╚══██╔══╝██║██╔════╝ ██╔════╝██╔══██╗    ██╔═══██╗██╔════╝
   ██║   ██║██║  ███╗█████╗  ██████╔╝    ██║   ██║███████╗
   ██║   ██║██║   ██║██╔══╝  ██╔══██╗    ██║   ██║╚════██║
   ██║   ██║╚██████╔╝███████╗██║  ██║    ╚██████╔╝███████║
   ╚═╝   ╚═╝ ╚═════╝ ╚══════╝╚═╝  ╚═╝     ╚═════╝ ╚══════╝
```

**Bengal Tiger OS** is an enhanced 32-bit hobby operating system written in C and x86 assembly. It boots via GRUB, features a colorful terminal interface, and provides a rich set of built-in commands. This OS is designed for learning OS development concepts and serves as a foundation for further experimentation.

---

## ✨ New in Version 0.3.0

### Core Enhancements
- 🎨 **Enhanced Shell** - Command history (Up/Down arrows), cursor movement, colorful output
- ⏱️ **Timer Subsystem** - Accurate uptime tracking, sleep functions
- 🧠 **Heap Allocator** - Dynamic memory with `kmalloc`/`kfree`, corruption detection
- 🔴 **Panic Handler** - Informative crash screens with register dump
- 🔍 **PCI Scanner** - Detects and lists all PCI devices

### New Commands
| Command | Description |
|---------|-------------|
| `neofetch` | Display system info with ASCII art |
| `uptime` | Show system uptime |
| `mem` | Display memory usage statistics |
| `pci` | List detected PCI devices |
| `history` | Show command history |
| `color <n>` | Change text color (0-15) |
| `reboot` | Reboot the system |
| `shutdown` | Halt the system |

### Keyboard Improvements
- **Shift + Letter** for uppercase
- **Caps Lock** support
- **Arrow keys** for navigation
- **Home/End** jump to line start/end
- **Delete** key support
- **Backspace** works properly

---

## 🏗️ Architecture

```
┌─────────────────────────────────────────────────────────────┐
│                      USER SHELL                              │
│  ┌─────────────────────────────────────────────────────────┐│
│  │  Commands: help, neofetch, pci, mem, uptime, etc.       ││
│  └─────────────────────────────────────────────────────────┘│
├─────────────────────────────────────────────────────────────┤
│                    KERNEL SUBSYSTEMS                         │
│  ┌──────────┐ ┌──────────┐ ┌──────────┐ ┌──────────┐       │
│  │  Timer   │ │ Keyboard │ │   Heap   │ │   FAT    │       │
│  │  (PIT)   │ │  (PS/2)  │ │ Allocator│ │Filesystem│       │
│  └──────────┘ └──────────┘ └──────────┘ └──────────┘       │
├─────────────────────────────────────────────────────────────┤
│                  HARDWARE ABSTRACTION                        │
│  ┌──────────┐ ┌──────────┐ ┌──────────┐ ┌──────────┐       │
│  │   IDT    │ │   PIC    │ │  Paging  │ │   PCI    │       │
│  │  (256)   │ │  (8259)  │ │  (4MB)   │ │  Scanner │       │
│  └──────────┘ └──────────┘ └──────────┘ └──────────┘       │
├─────────────────────────────────────────────────────────────┤
│                     BOOT (GRUB/Multiboot)                    │
└─────────────────────────────────────────────────────────────┘
```

---

## 📁 Project Structure

```
Bengal-Tiger-OS/
├── boot/
│   └── boot.s              # Multiboot entry point
├── grub/
│   └── grub.cfg            # GRUB bootloader config
├── kernel/
│   ├── main.c              # Kernel entry point
│   ├── common.c/h          # Utility functions, types
│   ├── idt.c/h             # Interrupt Descriptor Table
│   ├── isr.s               # Interrupt service routines (asm)
│   ├── isr.c               # Interrupt handlers (C)
│   ├── pic.c/h             # 8259 PIC controller
│   ├── paging.c/h          # Virtual memory management
│   ├── heap.c/h            # Dynamic memory allocator
│   ├── timer.c/h           # PIT timer driver
│   ├── keyboard.c/h        # PS/2 keyboard driver
│   ├── disk.c/h            # ATA disk driver
│   ├── fat.c/h             # FAT filesystem (stub)
│   ├── pci.c/h             # PCI bus scanner
│   ├── nic.c/h             # Network driver (stub)
│   ├── scheduler.c/h       # Task scheduler (stub)
│   ├── shell.c/h           # Command-line interface
│   └── panic.c/h           # Kernel panic handler
├── linker.ld               # Linker script
├── makefile                # Build system
└── docs/                   # Documentation
```

---

## 🛠️ Build Instructions

- Check the [BUILD.md](docs/BUILD.md) file for detailed build instructions.

## 📖 Using Bengal Tiger OS

### First Boot
1. GRUB menu appears (5 second timeout)
2. Bengal Tiger logo animation plays
3. Enter your username when prompted
4. Shell prompt appears: `yourname@bengal-tiger:~$`

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

##  Contact

Created with 💛 by BengalEmpire

*Roar like a tiger, code like a pro! 🐯*

# Bengal Tiger OS - Build System Documentation

## Overview

Bengal Tiger OS uses a GNU Makefile-based build system. The build process compiles 32-bit x86 code and creates a bootable ISO image compatible with GRUB.

---

## Toolchain Requirements

### Linux (Debian/Ubuntu)

```bash
# Essential build tools
sudo apt install build-essential

# 32-bit support on 64-bit systems
sudo apt install gcc-multilib

# GRUB and ISO creation
sudo apt install grub-pc-bin grub-common xorriso

# Emulation
sudo apt install qemu-system-x86
```

### macOS

```bash
# Install Homebrew if not already installed
/bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"

# Install cross-compiler (recommended)
brew install i686-elf-gcc i686-elf-binutils

# Install GRUB and tools
brew install xorriso grub

# Install QEMU
brew install qemu
```

### Windows (WSL - Recommended)

1. Install WSL from Microsoft Store (Ubuntu recommended)
2. Open Ubuntu terminal
3. Follow Linux instructions above

### Windows (MSYS2)

```bash
# Update MSYS2
pacman -Syu

# Install build tools
pacman -S make gcc binutils

# For 32-bit cross-compilation
pacman -S mingw-w64-i686-gcc

# GRUB and utilities
pacman -S grub xorriso

# QEMU
pacman -S mingw-w64-x86_64-qemu
```

---

## Build Commands

### Basic Commands

| Command | Description |
|---------|-------------|
| `make` | Build kernel and create ISO |
| `make all` | Same as `make` |
| `make clean` | Remove all build artifacts |
| `make run` | Build and run in QEMU |
| `make debug` | Build and run with GDB server |
| `make help` | Show available targets |

### Build Output

```
bengaltiger.iso          # Bootable ISO image
build/
├── boot.o               # Compiled boot assembly
├── main.o               # Kernel entry point
├── common.o             # Utility functions
├── idt.o                # IDT setup
├── isr.o                # ISR assembly stubs
├── isr_c.o              # ISR C handlers
├── pic.o                # PIC driver
├── paging.o             # Virtual memory
├── heap.o               # Memory allocator
├── keyboard.o           # Keyboard driver
├── timer.o              # Timer driver
├── disk.o               # Disk driver
├── fat.o                # FAT filesystem
├── scheduler.o          # Scheduler
├── shell.o              # Shell
├── pci.o                # PCI bus
├── nic.o                # Network stub
├── panic.o              # Panic handler
└── kernel.bin           # Linked kernel binary
```

---

## Compiler Flags Explained

```makefile
CFLAGS = -m32                    # Generate 32-bit code
         -I./kernel              # Include path for headers
         -fno-pic                # No position-independent code
         -fno-builtin            # Don't use GCC builtin functions
         -nostdlib               # Don't link standard library
         -nostartfiles           # Don't use standard startup files
         -nodefaultlibs          # Don't use default libraries
         -fno-stack-protector    # Disable stack canaries
         -Wall                   # Enable all warnings
         -Wextra                 # Enable extra warnings
         -Werror=implicit-function-declaration  # Error on missing prototypes
         -O2                     # Optimization level
         -g                      # Include debug symbols
```

### Why These Flags?

| Flag | Reason |
|------|--------|
| `-m32` | We're building a 32-bit OS |
| `-fno-builtin` | Kernel provides its own memcpy, strlen, etc. |
| `-nostdlib` | No C runtime or standard library in kernel mode |
| `-fno-stack-protector` | No __stack_chk_fail in freestanding environment |
| `-fno-pic` | Simplifies linking; we know load address |

---

## Linker Script (linker.ld)

```ld
ENTRY(start)                    /* Entry point symbol */

SECTIONS
{
    . = 0x00100000;             /* Load at 1MB (after BIOS area) */

    .text : AT(ADDR(.text))
    {
        KEEP(*(.multiboot))     /* Multiboot header MUST be first */
        *(.text)                /* Code */
        *(.rodata)              /* Read-only data */
    }

    .data : {
        *(.data)                /* Initialized data */
    }

    .bss : {
        *(.bss)                 /* Uninitialized data */
        *(COMMON)
        PROVIDE(bss_end = .);   /* Symbol for kernel to know end of BSS */
    }

    /DISCARD/ : { 
        *(.eh_frame)            /* Exception handling (not needed) */
        *(.comment)             /* Comments (not needed) */
    }
}
```

### Why 1MB?

- First 1MB contains BIOS routines and data
- Real mode and A20 gate considerations
- VGA buffer at 0xB8000
- Loading at 1MB is conventional for protected mode kernels

---

## Multiboot Header

Located in `boot/boot.s`, this header tells GRUB how to load the kernel:

```asm
.section .multiboot
.align 4
.long 0x1BADB002              /* Magic number */
.long 0x00010003              /* Flags: page-aligned, mem info, address fields */
.long -(0x1BADB002 + 0x00010003)  /* Checksum (magic + flags + checksum = 0) */
```

### Multiboot Flags Used

| Bit | Value | Meaning |
|-----|-------|---------|
| 0 | 0x001 | Page-align modules |
| 1 | 0x002 | Provide memory map |
| 16 | 0x10000 | Use address fields in header |

---

## Adding New Source Files

### Step 1: Create the source file

Create `kernel/mymodule.c` and `kernel/mymodule.h`

### Step 2: Update Makefile

Add to C_SOURCES:
```makefile
C_SOURCES += kernel/mymodule.c
```

Add to OBJS:
```makefile
OBJS = ... \
       build/mymodule.o
```

Add compilation rule:
```makefile
build/mymodule.o: kernel/mymodule.c
    $(CC) $(CFLAGS) -c -o $@ $<
```

### Step 3: Include in kernel

In `kernel/main.c`:
```c
#include "mymodule.h"

void kmain(...) {
    ...
    mymodule_init();
    ...
}
```

---

## Debugging with GDB

### Start Debug Session

```bash
# Terminal 1: Start QEMU with debug server
make debug

# Terminal 2: Connect GDB
gdb -ex "target remote :1234" build/kernel.bin
```

### Useful GDB Commands

```gdb
# Set breakpoint at function
break kmain
break shell_handler

# Continue execution
continue

# Step through code
step    # Step into
next    # Step over

# Examine memory
x/10x 0xB8000    # View VGA buffer
x/s 0x100000     # View as string

# View registers
info registers
print $eax

# Backtrace
bt

# Disassemble
disas kmain
```

### Example Debug Session

```gdb
(gdb) target remote :1234
(gdb) break kmain
(gdb) continue
Breakpoint 1, kmain () at kernel/main.c:42
(gdb) next
(gdb) print mbi->mem_upper
$1 = 130048
(gdb) continue
```

---

## Running in QEMU

### Basic Run

```bash
qemu-system-i386 -cdrom bengaltiger.iso -m 512M
```

### With FAT Disk for Persistence

```bash
qemu-system-i386 \
    -cdrom bengaltiger.iso \
    -m 512M \
    -drive file=fat:rw:iso/boot,format=raw \
    -boot d
```

### Additional QEMU Options

| Option | Description |
|--------|-------------|
| `-m 512M` | 512MB memory |
| `-serial stdio` | Serial output to terminal |
| `-d int` | Log interrupts |
| `-no-reboot` | Don't reboot on triple fault |
| `-monitor stdio` | QEMU monitor commands |
| `-s` | Enable GDB server on :1234 |
| `-S` | Start paused (wait for GDB) |

---

## Troubleshooting

### Build Errors

**Error: gcc cannot create 32-bit executable**
```bash
# Install multilib support
sudo apt install gcc-multilib libc6-dev-i386
```

**Error: grub-mkrescue not found**
```bash
sudo apt install grub-pc-bin grub-common xorriso
```

**Error: undefined reference to `__stack_chk_fail`**
- Ensure `-fno-stack-protector` is in CFLAGS

**Error: Kernel too large**
- Check if debug symbols are included (-g)
- Consider stripping: `strip build/kernel.bin`

### Runtime Errors

**GRUB "file not found"**
- Check grub.cfg path: `/boot/kernel.bin`
- Ensure ISO structure is correct

**Triple fault / reboot loop**
- Enable QEMU int logging: `-d int`
- Check for null pointer dereferences
- Verify GDT/IDT setup

**No keyboard input**
- Verify IRQ1 handler in IDT
- Check PIC initialization
- Verify keyboard port 0x60 read

---

## ISO Structure

```
iso/
├── boot/
│   ├── grub/
│   │   └── grub.cfg         # GRUB configuration
│   ├── kernel.bin           # Kernel binary
│   ├── config.cfg           # User configuration
│   └── hello.txt            # Sample file
```

### grub.cfg

```cfg
set timeout=5
set default=0

menuentry "Bengal Tiger OS" {
    multiboot /boot/kernel.bin
    boot
}
```

---

## Continuous Integration (Example)

### GitHub Actions Workflow

```yaml
name: Build Bengal Tiger OS

on: [push, pull_request]

jobs:
  build:
    runs-on: ubuntu-latest
    
    steps:
    - uses: actions/checkout@v3
    
    - name: Install dependencies
      run: |
        sudo apt update
        sudo apt install -y gcc-multilib grub-pc-bin xorriso
    
    - name: Build
      run: make all
    
    - name: Upload ISO
      uses: actions/upload-artifact@v3
      with:
        name: bengaltiger-iso
        path: bengaltiger.iso
```

---

*This document is part of Bengal Tiger OS v0.3.0*

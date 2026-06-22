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
sudo apt install grub-pc-bin grub-common xorriso mtools

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
├── cpu.o                # CPU initialization (A20, CPUID, FPU, SSE)
├── gdt.o                # Global Descriptor Table
├── rtc.o                # CMOS Real-Time Clock driver
├── serial.o             # 16550 UART serial driver
├── keyboard.o           # Keyboard driver
├── timer.o              # Timer driver
├── disk.o               # Enhanced ATA/ATAPI disk driver
├── fat.o                # FAT filesystem
├── vbe.o                # VBE framebuffer driver
├── font.o               # 8x16 bitmap font data
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

## GRUB Graphics Mode Configuration

When running in QEMU or on real hardware with VBE-compatible graphics, the GRUB menu offers multiple boot options:

```cfg
menuentry "Bengal Tiger OS (VGA Text 80x25)" {
    set gfxpayload=text
    multiboot /boot/kernel.bin
    boot
}

menuentry "Bengal Tiger OS (1024x768x32)" {
    set gfxpayload=1024x768x32
    multiboot /boot/kernel.bin
    boot
}

menuentry "Bengal Tiger OS (800x600x32)" {
    set gfxpayload=800x600x32
    multiboot /boot/kernel.bin
    boot
}

menuentry "Bengal Tiger OS (1280x1024x32)" {
    set gfxpayload=1280x1024x32
    multiboot /boot/kernel.bin
    boot
}
```

The `set gfxpayload` directive tells GRUB to switch to the specified VESA mode before loading the kernel. The framebuffer information is passed to the kernel via the Multiboot structure (flags bit 12).

---

## Boot Initialization Order

The kernel's `kmain()` follows a strict 7-phase initialization:

```
Phase 1: CPU Initialization
├── A20 Gate Enable (memory above 1MB)
├── GDT Setup (own descriptors, not GRUB's)
└── CPUID/FPU/SSE Detection & Init

Phase 2: Critical Hardware
├── PIC Remap (IRQ 0-15 → INT 32-47)
└── IDT Install (256 gates)

Phase 3: Memory Management
├── Memory Map Parsing (E820 from multiboot)
├── PMM Init (bitmap allocator)
├── Paging Enable (dynamic page tables)
├── Heap Init (kmalloc/kfree)
└── VBE Framebuffer Init (from multiboot info)

Phase 4: Device Drivers
├── Serial (COM1 debug output)
├── Timer (PIT at 100Hz)
├── Keyboard (PS/2)
├── RTC (CMOS clock)
├── ATA Disk (IDENTIFY + PIO)
├── FAT Filesystem
├── Scheduler
├── PCI Scanner
└── NIC

Phase 5: Enable Interrupts (STI)

Phase 6: User Setup (config → shell)

Phase 7: Idle Loop (HLT)
```

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

### Step 4: Update documentation

- Update `CHANGELOG.md` with new feature details
- Add API section to `docs/API.md`
- Update project structure in `readme.md`
- Update roadmap in `docs/ROADMAP.md`

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
(gdb) print mbi->flags
$1 = 4096
(gdb) continue
```

---

## Running in QEMU

### Basic Run

```bash
qemu-system-i386 -cdrom bengaltiger.iso -m 512M
```

### With Serial Debug Output

```bash
qemu-system-i386 \
    -cdrom bengaltiger.iso \
    -m 512M \
    -serial stdio
```

### With FAT Disk for Persistence

```bash
qemu-system-i386 \
    -cdrom bengaltiger.iso \
    -m 512M \
    -drive file=fat:rw:iso/boot,format=raw \
    -boot d
```

### With Graphics Mode

When running via `make run`, QEMU is launched with `-vga std` which provides standard VGA/VBE support. After booting, select a graphics mode entry from the GRUB menu (e.g., "Bengal Tiger OS (1024x768x32)") to access the VBE framebuffer.

### With Serial + Graphics

```bash
qemu-system-i386 \
    -cdrom bengaltiger.iso \
    -m 512M \
    -serial stdio \
    -vga std
```

### Additional QEMU Options

| Option | Description |
|--------|-------------|
| `-m 512M` | 512MB memory |
| `-serial stdio` | Serial output to terminal (115200 baud) |
| `-d int` | Log interrupts |
| `-no-reboot` | Don't reboot on triple fault |
| `-monitor stdio` | QEMU monitor commands |
| `-s` | Enable GDB server on :1234 |
| `-S` | Start paused (wait for GDB) |

### Debug via Serial

Connect to COM1 at 115200 baud, 8N1 to see kernel boot logs in real time:

```bash
# QEMU
qemu-system-i386 -cdrom bengaltiger.iso -serial stdio

# On real hardware, use a null-modem cable and terminal program:
screen /dev/ttyS0 115200
```

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
sudo apt install grub-pc-bin grub-common xorriso mtools
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
- Check A20 gate is enabled

**No keyboard input**
- Verify IRQ1 handler in IDT
- Check PIC initialization
- Verify keyboard port 0x60 read

**No serial output**
- Ensure QEMU is launched with `-serial stdio`
- Terminal program configured for 115200 baud, 8N1
- Check COM port base address (default COM1 = 0x3F8)

**Graphics mode not working**
- Ensure QEMU is launched with `-vga std`
- Select a graphics mode entry from GRUB menu (not the text mode entry)
- Verify the multiboot header has flag bit 2 set (0x04) for VBE info
- Run `vbe` command in shell to check framebuffer status

**ATA disk not detected**
- Run `disk` command to check IDENTIFY data
- Check ATA cable/connection on real hardware
- Verify disk is powered on

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

menuentry "Bengal Tiger OS (VGA Text 80x25)" {
    set gfxpayload=text
    multiboot /boot/kernel.bin
    boot
}

menuentry "Bengal Tiger OS (1024x768x32)" {
    set gfxpayload=1024x768x32
    multiboot /boot/kernel.bin
    boot
}

menuentry "Bengal Tiger OS (Debug - VGA Text)" {
    set gfxpayload=text
    multiboot /boot/kernel.bin debug
    boot
}

menuentry "Halt System" {
    halt
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
        sudo apt install -y gcc-multilib grub-pc-bin xorriso mtools
    
    - name: Build
      run: make all
    
    - name: Upload ISO
      uses: actions/upload-artifact@v3
      with:
        name: bengaltiger-iso
        path: bengaltiger.iso
```

---

*This document is part of Bengal Tiger OS v0.5.0*

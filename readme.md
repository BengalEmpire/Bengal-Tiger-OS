# Bengal Tiger OS

Bengal Tiger OS is an enhanced prototype of a 32-bit hobby operating system. It boots via GRUB, displays an animated ASCII art logo, performs a one-time username setup on first boot (stored in a fixed sector), and provides a terminal-based shell with basic commands. No GUI is included or needed. The OS is interrupt-driven, with basic memory management, disk I/O, and a stub filesystem.

### New Features
- **Animated Logo**: Simple line-by-line reveal of ASCII tiger logo with delays.
- **First-Time Setup**: On first boot, prompts for username and saves it to a config sector (persistent if disk allows write).
- **Shell Enhancements**: Username in prompt (e.g., `user@bengal-tiger:~$`), added commands: `echo <msg>`, `clear`, `exit`.
- **Persistence Hack**: Uses fixed sector 2 for config.cfg (not full FAT write).
## Build Instructions

### Linux (Debian/Ubuntu)
1. Install dependencies:
   ```
   sudo apt update
   sudo apt install build-essential gcc-multilib grub-pc-bin grub-common xorriso qemu-system-x86
   ```
2. Build the ISO:
   ```
   make all
   ```
3. Run in QEMU:
   ```
   make run
   ```
   - For write support (persistence), QEMU uses `-drive file=fat:rw:iso/boot,format=raw` in Makefile.

### Windows
Use WSL (Windows Subsystem for Linux) or MSYS2.
- **WSL (Recommended)**:
  1. Install WSL (Ubuntu) via Microsoft Store.
  2. Open WSL terminal, follow Linux instructions above.
- **MSYS2**:
  1. Download/install MSYS2 from msys2.org.
  2. Open MSYS2 terminal, install tools:
     ```
     pacman -Syu
     pacman -S make gcc binutils grub xorriso qemu
     ```
  3. Note: May need `mingw-w64-i686-gcc` for -m32.
  4. Run `make all` and `qemu-system-i386 -cdrom bengaltiger.iso -m 512M`.

## Run Instructions

### VirtualBox
1. Create a new VM:
   - Name: Bengal Tiger OS
   - Type: Other, Version: Other/Unknown (32-bit)
   - Memory: 512MB
   - Hard Disk: None (or create VDI for persistence, but ISO is CD)
2. Settings:
   - System: Enable PAE/NX if issues.
   - Storage: Add IDE Controller, attach `bengaltiger.iso` as CD-ROM.
3. Start VM: It boots GRUB, loads kernel, shows logo, setup if first, then shell.
- For persistence: Create a VDI HDD, attach as IDE master. But current disk code assumes IDE disk 0; may need tweaks.

### VMware
1. Create VM:
   - Compatibility: Workstation 17.x
   - Guest OS: Other 32-bit
   - Memory: 512MB
   - Hard Disk: None or SCSI/IDE VMDK.
   - CD-ROM: Use ISO `bengaltiger.iso`.
2. Power on: Similar to VirtualBox.

### Real Machine
1. Write ISO to USB/CD:
   - Linux: `sudo dd if=bengaltiger.iso of=/dev/sdX bs=4M status=progress && sync` (replace sdX with USB device).
   - Windows: Use Rufus/Etcher.
2. Boot from USB/CD in BIOS (legacy mode, not UEFI).
3. Note: Real hardware may have issues with ATA/keyboard; test in VM first. Persistence works if HDD present, but code assumes primary IDE.

## Testing Guide
- Boot: See GRUB (5s timeout), then logo animation (tiger ASCII reveals line-by-line).
- First Boot: Prompt "Enter username:", type and Enter. Saved for next boots.
- Shell: Prompt like `yourname@bengal-tiger:~$`
  - `help`: List commands.
  - `ls`: Show files.
  - `cat hello.txt`: Show content.
  - `echo test`: Print "test".
  - `clear`: Clear screen.
  - `exit`: Halt OS.
- Issues: If no input, check IRQ33. If no write, check disk setup in VM (needs writable disk).

## Roadmap
- Full FAT support (dir traversal, write).
- Multitasking (real switching).
- Graphics: Add VBE for .raw logo animation (use multiboot VBE info, map FB, copy raw data frames with delays).
- More Commands: `mkdir`, `rm`, `cd`.
- User Mode: Ring 3 tasks.
- Networking: Implement NIC driver.

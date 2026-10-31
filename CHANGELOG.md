# Changelog

All notable changes to **Bengal Tiger OS** are documented in this file.

## [1.0.0] - 2026-10-31 ("Royal Bengal")

### Added
- **Unified Console & Desktop (`shell.c`, `vbe.c`, `editor.c`)**:
  - Full 24/32-bpp VBE graphical terminal (`1024x768x32` default) with top live status bar, hardware-adapted RGB packing, fast region scrolling, simultaneous VGA `80x25` hardware cursor support, and full VT100/ANSI COM1 serial terminal mirroring.
  - Interactive full-screen text editor (`edit <file>`) with 2D navigation, line split/join, unsaved-change protection, and direct FAT12/FAT16 saving (`Ctrl+S`).
  - Interactive VBE Graphical Desktop Workspace (`gui` / `F2`) with PS/2 mouse cursor tracking, live memory/CPU/task monitor, filesystem explorer, and gateway ping action.
  - Shell scripting (`sh <script.sh>`), output redirection (`>` / `>>`), Tab completion for commands & files, recursive-descent expression calculator (`calc`), `top`, `spawn`, `dmesg`, `hexdump`, `stat`, `df`, `mkfs`, `cp`, `mv`, `rm`, `append`, and `beep`.
- **Real TCP/IPv4 Networking (`nic.c`, `net.c`)**:
  - Intel PRO/1000 (`E1000`: `82540EM`/`82545EM`/`82574L`) MMIO driver alongside static-DMA Realtek `RTL8139` driver.
  - Complete Ethernet II, ARP cache & request/reply engine, IPv4 subnet routing, UDP datagrams, 4-way DHCPv4 DORA client (`dhcp` + boot autoconfig), and live ICMP Echo Request/Reply (`ping`).
- **Storage & Filesystems (`disk.c`, `fat.c`)**:
  - 4-drive Primary/Secondary IDE ATA/ATAPI scanner distinguishing optical boot media from writable hard disks.
  - Automatic 512 KB Live FAT12 RAMDisk seeded with starter files & `DEMO.SH` when booting from ISO/USB without a formatted disk.
  - `mkfs` volume formatter (FAT12 `<4085` clusters / FAT16 `>=4085` clusters) and MBR partition table (`0x1BE`) + Superfloppy mounting.
- **Kernel Diagnostics & Automated Verification**:
  - Timestamped 8 KB circular kernel log (`klog.c` / `dmesg`).
  - Automated ELF32/Multiboot1 binary verifier (`tests/verify_kernel_binary.py`), ASan/UBSan host subsystem unit test suite (`tests/test_host.c`), and two-boot QEMU hardware E2E test (`tests/qemu_smoke_test.py`).

### Fixed
- Fixed broken Multiboot1 `AOUT_KLUDGE` flag (`0x10000`) on ELF32 image, misaligned `0x9BFFF` EBDA stack, missing `cld` direction flag, and unmerged `.rodata.*` linker sections.
- Decoupled keyboard/serial IRQ handlers from shell command execution so `hlt`/`sleep_ms`/`ping`/`dhcp`/`edit` run safely with `IF=1` without CPU deadlocks.
- Fixed PS/2 mouse initialization leaving Keyboard Port 1 disabled on systems without an auxiliary mouse, and added 0xE0 extended scancode handling.
- Fixed CMOS RTC interrupt-reenable bug during early boot, inverted Status B BCD flag (`0x04`), and 12-hour PM bit handling.
- Fixed E820 physical memory reservation, dynamic MMIO page table allocation above 4 MB, heap `kmalloc_aligned` metadata unwrapping in `kfree()`, and `INT 0x80` scheduler yield (`SYS_YIELD`).
- Fixed Secondary ATA control port (`0x376`), multi-cluster FAT chain freeing on overwrite/delete, and non-destructive A20 memory wrap-around check.

## [0.5.0] - Initial Development Milestones
- Early Multiboot prototype, GDT/IDT/PIC setup, basic PMM/paging/heap, PS/2 input, ATA PIO, and VGA text shell.

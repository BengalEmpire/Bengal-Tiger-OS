# Bengal Tiger OS - Development Roadmap

## Version History

| Version | Date | Highlights |
|---------|------|------------|
| 0.1.0 | - | Initial prototype, basic boot |
| 0.2.0 | - | Shell, keyboard, first-time setup |
| 0.3.0 | 2025-12-26 | Enhanced shell, heap, timer, PCI, docs |
| **0.4.0** | **2026-06-18** | **Real hardware readiness: GDT, A20, CPUID, FPU, RTC, Serial, enhanced ATA** |

---

## Current Version: 0.4.0 — "Real Hardware Ready"

### Completed Features ✅

#### Real Hardware Boot Stability
- [x] A20 Gate Enable — Fast gate + keyboard controller fallback
- [x] Own GDT — Flat 32-bit model, independent of GRUB
- [x] Stack Initialization — Explicit ESP in boot.s
- [x] CPUID Detection — Vendor, brand, family/model/stepping
- [x] FPU Initialization — CR0 config + FNINIT
- [x] SSE/SSE2 Support — CR4 OSFXSR/OSXMMEXCPT bits
- [x] Full Multiboot Memory Map — E820 parsing (not just mem_upper)
- [x] Dynamic Page Tables — After kernel BSS, not hardcoded 0x9C000

#### New Drivers
- [x] RTC (CMOS Real-Time Clock) — Full date/time reading
- [x] Serial Port (16550 UART) — COM1 debug output at 115200 baud
- [x] Enhanced ATA/ATAPI — IDENTIFY command, model/serial/firmware, LBA48
- [x] ACPI Shutdown — Multiple VM/hardware shutdown methods

#### New Shell Commands
- [x] `date` — Real date/time from RTC
- [x] `disk` — ATA drive information
- [x] `cpu` — CPU vendor, brand, features

#### Enhanced Commands
- [x] `reboot` — PS/2 controller + ACPI + triple fault fallback
- [x] `shutdown` — QEMU + VirtualBox + Bochs ACPI methods

#### Build System
- [x] GNU Make based
- [x] Bootable ISO generation
- [x] QEMU integration
- [x] Debug support with GDB
- [x] Updated for 22 object files

#### Documentation (v0.4.0)
- [x] README updated with new features, architecture, 7-phase boot
- [x] Architecture docs updated with CPU init, GDT, new drivers
- [x] API docs updated with GDT, CPU, RTC, Serial, Disk APIs
- [x] Build docs updated with new files, serial debug, troubleshooting
- [x] Changelog with all v0.4.0 additions
- [x] Roadmap updated with v0.4.0 completed items

---

## Version 0.5.0 (Next Release)

### Priority: ★★★ High

#### Real Filesystem Support
- [ ] FAT12 filesystem driver
  - [ ] Boot sector parsing
  - [ ] FAT table reading
  - [ ] Directory traversal
  - [ ] File reading (full implementation)
  - [ ] File writing
  - [ ] File creation/deletion
- [ ] FAT16 filesystem support
- [ ] FAT32 filesystem support

#### New Shell Commands
- [ ] `mkdir <dir>` - Create directory
- [ ] `rm <file>` - Remove file
- [ ] `cd <dir>` - Change directory
- [ ] `pwd` - Print working directory
- [ ] `touch <file>` - Create empty file
- [ ] `write <file>` - Write to file

### Priority: ★★ Medium

#### Enhanced PCI
- [ ] Enable/disable devices
- [ ] Configure interrupts
- [ ] Memory-mapped I/O support

#### PS/2 Mouse Driver
- [ ] IRQ12 handler (INT 44)
- [ ] Mouse packet parsing
- [ ] Cursor rendering on VGA

#### Keyboard Enhancements
- [ ] Multi-scancode set support
- [ ] Media/extra keys
- [ ] Typematic rate configuration

---

## Version 0.6.0

### Priority: ★★★ High

#### Multitasking
- [ ] Task Control Block (TCB) structure
- [ ] Context switching
- [ ] Round-robin scheduler
- [ ] Preemptive multitasking
- [ ] Kernel threads

#### System Calls
- [ ] INT 0x80 handler
- [ ] Basic syscall table
- [ ] Fork/Exit/Wait
- [ ] Read/Write/Open/Close

### Priority: ★★ Medium

#### User Mode
- [ ] Ring 3 transition
- [ ] TSS setup
- [ ] User stack
- [ ] Syscall return to user mode

#### ELF Loader
- [ ] ELF header parsing
- [ ] Program header handling
- [ ] Memory mapping
- [ ] Entry point execution

---

## Version 0.7.0

### Graphics

#### VBE Support
- [ ] VESA BIOS Extensions
- [ ] Mode switching
- [ ] Linear framebuffer access
- [ ] 32-bit color modes

#### Basic GUI Framework
- [ ] Pixel drawing primitives
- [ ] Line/rectangle/circle
- [ ] Font rendering
- [ ] Window management basics

### Peripherals

#### Sound
- [ ] PC Speaker beeps
- [ ] AC97 driver (advanced)

---

## Version 1.0.0 (Major Release)

### Full GUI Desktop

- [ ] Desktop with icons
- [ ] Window manager
- [ ] Menu system
- [ ] File manager application
- [ ] Text editor
- [ ] Terminal emulator

### Networking

#### Network Stack
- [ ] Ethernet frame handling
- [ ] ARP protocol
- [ ] IPv4
- [ ] ICMP (ping)
- [ ] UDP
- [ ] TCP
- [ ] DHCP client

#### Network Drivers
- [ ] Intel E1000 (QEMU default)
- [ ] RTL8139 (common in VMs)
- [ ] Virtio-net

### Storage

- [ ] Full FAT16/FAT32 support
- [ ] ISO 9660 (CD-ROM filesystem)
- [ ] Initial ramdisk
- [ ] AHCI/SATA driver

---

## Future Considerations

### Long-term Goals

#### x86-64 Port
- [ ] Long mode transition
- [ ] 64-bit addressing
- [ ] New paging structures (PML4)
- [ ] Updated toolchain

#### UEFI Boot
- [ ] GOP framebuffer
- [ ] UEFI applications
- [ ] Secure Boot (optional)

#### Advanced Features
- [ ] SMP (Symmetric Multiprocessing)
- [ ] ACPI power management
- [ ] USB support (UHCI/EHCI)

### Possible Applications

- [ ] Web browser (text-based initially)
- [ ] IRC client
- [ ] Image viewer
- [ ] Music player
- [ ] Basic games

---

## Contributing Guidelines

### How to Contribute

1. **Bug Fixes** - Always welcome!
2. **New Drivers** - Follow existing patterns
3. **Shell Commands** - Easy starting point
4. **Documentation** - Improve clarity

### Code Style

- Use 4-space indentation
- Doxygen-style comments for functions
- Header guards: `#ifndef FILENAME_H`
- Prefix global functions with module name
- Use `UNUSED(x)` for unused parameters

### Commit Messages

```
[module] Brief description

Longer explanation if needed.
Fixes #issue_number
```

### Testing

Before submitting:
1. `make clean && make all` succeeds
2. Basic boot test in QEMU
3. Affected commands work correctly
4. No compiler warnings (`-Wall -Wextra` clean)
5. Serial boot log shows all OK

---

## Known Issues

| Issue | Description | Workaround |
|-------|-------------|------------|
| Tab completion | Not implemented | Type full command |
| Long commands | May overflow buffer | Keep under 256 chars |
| FAT write | Uses fixed sectors | For demo only |
| No multitasking | Single-threaded kernel | N/A for current use |
| No DMA | ATA uses PIO mode | Slower but compatible |

---

*Last updated: Bengal Tiger OS v0.4.0*

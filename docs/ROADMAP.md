# Bengal Tiger OS - Development Roadmap

## Version History

| Version | Date | Highlights |
|---------|------|------------|
| 0.1.0 | - | Initial prototype, basic boot |
| 0.2.0 | - | Shell, keyboard, first-time setup |
| 0.3.0 | 2025-12-26 | Enhanced shell, heap, timer, PCI, docs |
| 0.4.0 | 2026-06-18 | Real hardware readiness: GDT, A20, CPUID, FPU, RTC, Serial, enhanced ATA |
| **0.5.0** | **2026-06-18** | **VBE framebuffer graphics: 1024x768x32, pixel/text drawing, font** |

---

## Current Version: 0.5.0 — "Graphics Mode"

### Completed Features ✅

#### VBE Framebuffer Graphics
- [x] VBE linear framebuffer init from multiboot info (resolution, pitch, bpp, color channels)
- [x] Framebuffer physical memory mapping via paging_map()
- [x] Multiple resolution support: 1024x768x32, 800x600x32, 1280x1024x32
- [x] Pixel drawing: vbe_putpixel(), vbe_getpixel()
- [x] Shape drawing: vbe_fill_rect(), vbe_draw_rect(), vbe_draw_hline(), vbe_draw_vline()
- [x] Screen management: vbe_clear(), vbe_clear_color(), vbe_scroll()
- [x] Text rendering with 8x16 bitmap font (95 printable ASCII chars)
- [x] Word wrap and newline handling in vbe_draw_string()
- [x] 16 standard VGA colors pre-mapped to 32-bit RGBA
- [x] Color conversion: vbe_vga_to_rgb(), vbe_rgb() inline

#### GRUB Graphics Mode Support
- [x] gfxpayload configuration in grub.cfg
- [x] Multiple boot entries: Text, 1024x768x32, 800x600x32, 1280x1024x32
- [x] Fallback to VGA text mode when framebuffer not available

#### New Shell Commands
- [x] `vbe` — Show framebuffer info (resolution, bpp, pitch, color layout, address, size)
- [x] `vbe` — Draw demo pattern: 16 color bars, cyan border, welcome text
- [x] `neofetch` — Shows graphics resolution when VBE is active

#### New Files
- [x] `kernel/font.h` — 8x16 bitmap font header
- [x] `kernel/font.c` — 8x16 bitmap font data (95 characters × 16 bytes)
- [x] `kernel/vbe.h` — VBE framebuffer types, color definitions, drawing API
- [x] `kernel/vbe.c` — VBE init, pixel/rect/text rendering, paging mapping
- [x] `makefile` — Added vbe.o and font.o to build

#### Documentation (v0.5.0)
- [x] README updated with VBE features, architecture, commands
- [x] Architecture docs with graphics layer section
- [x] API docs with VBE and Font sections
- [x] Build docs with graphics mode info and new build files
- [x] Changelog with all v0.5.0 additions
- [x] Roadmap updated with v0.5.0 completed items

---

## Version 0.6.0 (Next Release)

### Priority: ★★★ High

#### Double-Buffering & Cursor
- [ ] Software double-buffer for flicker-free drawing
- [ ] Hardware cursor management
- [ ] Screen redraw optimization

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

#### PS/2 Mouse Driver
- [ ] IRQ12 handler (INT 44)
- [ ] Mouse packet parsing
- [ ] Cursor rendering on framebuffer

#### Keyboard Enhancements
- [ ] Multi-scancode set support
- [ ] Media/extra keys
- [ ] Typematic rate configuration

---

## Version 0.7.0

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

## Version 0.8.0

### Enhanced PCI
- [ ] Enable/disable devices
- [ ] Configure interrupts
- [ ] Memory-mapped I/O support

### Basic GUI Framework
- [ ] Desktop with icons
- [ ] Window management basics
- [ ] Menu system
- [ ] File manager application
- [ ] Text editor
- [ ] Terminal emulator

### Peripherals
- [ ] PC Speaker beeps
- [ ] AC97 driver (advanced)

---

## Version 1.0.0 (Major Release)

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
| VBE no text fallback | Can't switch back to text mode | Reboot with text entry |

---

*Last updated: Bengal Tiger OS v0.5.0*

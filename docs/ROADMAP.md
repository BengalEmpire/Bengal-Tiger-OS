# Bengal Tiger OS - Development Roadmap

## Version History

| Version | Date | Highlights |
|---------|------|------------|
| 0.1.0 | - | Initial prototype, basic boot |
| 0.2.0 | - | Shell, keyboard, first-time setup |
| 0.3.0 | Current | Enhanced shell, heap, timer, PCI, docs |

---

## Current Version: 0.3.0

### Completed Features ✅

#### Kernel Core
- [x] Multiboot-compliant bootloader integration
- [x] Protected mode (32-bit) operation
- [x] IDT with 256 entries
- [x] CPU exception handling with panic screen
- [x] Hardware IRQ handling (timer, keyboard)
- [x] PIC remapping (8259A)

#### Memory Management
- [x] Physical memory manager (bitmap allocator)
- [x] Paging enabled (identity-mapped 4MB)
- [x] Kernel heap (kmalloc/kfree)
- [x] Memory corruption detection

#### Drivers
- [x] PS/2 Keyboard with full modifier support
- [x] PIT Timer at 100Hz
- [x] ATA PIO disk access (basic)
- [x] PCI bus scanner

#### Shell
- [x] Command-line interface
- [x] Command history (Up/Down arrows)
- [x] Cursor movement (Left/Right/Home/End)
- [x] Colored output
- [x] 15+ built-in commands

#### Build System
- [x] GNU Make based
- [x] Bootable ISO generation
- [x] QEMU integration
- [x] Debug support with GDB

#### Documentation
- [x] README with usage guide
- [x] Architecture documentation
- [x] Build system documentation
- [x] API reference

---

## Version 0.4.0 (Next Release)

### Priority: ★★★ High

#### Real Filesystem Support
- [ ] FAT12 filesystem driver
  - [ ] Boot sector parsing
  - [ ] FAT table reading
  - [ ] Directory traversal
  - [ ] File reading (full implementation)
  - [ ] File writing
  - [ ] File creation/deletion

#### New Shell Commands
- [ ] `mkdir <dir>` - Create directory
- [ ] `rm <file>` - Remove file
- [ ] `cd <dir>` - Change directory
- [ ] `pwd` - Print working directory
- [ ] `touch <file>` - Create empty file
- [ ] `write <file>` - Write to file

### Priority: ★★ Medium

#### Real Time Clock
- [ ] CMOS RTC driver
- [ ] `date` command with actual date/time
- [ ] Timezone support

#### Serial Port
- [ ] COM1/COM2 driver
- [ ] Kernel debug output via serial
- [ ] Serial console support

#### Enhanced PCI
- [ ] Enable/disable devices
- [ ] Configure interrupts
- [ ] Memory-mapped I/O support

---

## Version 0.5.0

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

## Version 0.6.0

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

#### PS/2 Mouse
- [ ] Mouse packet handling
- [ ] Cursor rendering
- [ ] Click events

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

---

## Future Considerations

### Long-term Goals

#### x86-64 Port
- [ ] Long mode transition
- [ ] 64-bit addressing
- [ ] New paging structures
- [ ] Updated toolchain

#### UEFI Boot
- [ ] GOP framebuffer
- [ ] UEFI applications
- [ ] Secure Boot (optional)

#### Advanced Features
- [ ] SMP (Symmetric Multiprocessing)
- [ ] ACPI power management
- [ ] USB support
- [ ] AHCI/SATA driver

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
4. No compiler warnings

---

## Known Issues

| Issue | Description | Workaround |
|-------|-------------|------------|
| Tab completion | Not implemented | Type full command |
| Long commands | May overflow buffer | Keep under 256 chars |
| FAT write | Uses fixed sectors | For demo only |
| Real hardware | ATA timing issues | Use VM |

---

*Last updated: Bengal Tiger OS v0.3.0*

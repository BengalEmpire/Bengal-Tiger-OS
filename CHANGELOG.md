# Changelog

All notable changes to Bengal Tiger OS will be documented in this file.


## [0.3.0] - 2025-12-26

### Added

#### Kernel Core
- **Kernel Heap Allocator** - Dynamic memory allocation with `kmalloc()`, `kfree()`, `krealloc()`, `kzalloc()`
  - First-fit free-list algorithm
  - Block coalescing for defragmentation
  - Magic number corruption detection
  - Memory statistics tracking

- **Timer Subsystem** - Accurate timing services
  - PIT configured at 100Hz (10ms tick)
  - `sleep_ms()` and `sleep_s()` functions
  - Uptime tracking in ticks, seconds, and formatted string
  - Decoupled from scheduler module

- **Kernel Panic Handler** - Informative crash handling
  - Red panic screen with error message
  - Full CPU register dump
  - Page fault address display (CR2)
  - `ASSERT()` and `PANIC()` macros

- **PCI Bus Scanner** - Hardware detection
  - Full bus/device/function enumeration
  - Vendor and class name lookup
  - Device ID, BAR, and IRQ tracking
  - `pci` shell command to list devices

#### Shell Enhancements
- **Command History** - Navigate with Up/Down arrow keys
- **Cursor Movement** - Left/Right/Home/End key support
- **Delete Key** - Delete character at cursor
- **New Commands:**
  - `neofetch` - System info with ASCII art
  - `uptime` - Show system uptime
  - `mem` - Memory usage statistics
  - `pci` - List PCI devices
  - `history` - Show command history
  - `color <n>` - Change text color (0-15)
  - `reboot` - Reboot the system
  - `shutdown` - Halt the system

#### Keyboard Driver
- **Shift Key Support** - Uppercase letters and symbols
- **Caps Lock** - Toggle case for letters
- **Ctrl/Alt Detection** - Modifier key tracking
- **Special Keys** - Arrow keys, Home, End, Delete
- **Ring Buffer** - Asynchronous key input

#### Documentation
- **README.md** - Comprehensive usage guide
- **docs/ARCHITECTURE.md** - System design documentation
- **docs/BUILD.md** - Build system guide with troubleshooting
- **docs/API.md** - Complete API reference
- **docs/ROADMAP.md** - Future development plans

### Changed

- **Makefile** - Complete rewrite
  - Better organization
  - New targets: `debug`, `help`
  - Improved compiler flags
  - Debug symbol support

- **Shell Prompt** - Now colorful with username highlighting
  - Green username
  - Cyan hostname
  - White separators

- **Boot Sequence** - Added boot animation
  - ASCII tiger art
  - Progress messages with status

- **common.h** - Additional utilities
  - `memcmp()`, `strcpy()`, `strncpy()`, `strcat()`, `strchr()`
  - `MIN()`, `MAX()`, `ALIGN_UP()`, `ALIGN_DOWN()` macros
  - `UNUSED()` macro
  - Optimized `memset()` and `memcpy()`

- **PIC Driver** - Enhanced interrupt controller handling
  - IRQ masking functions
  - Spurious IRQ detection
  - Proper EOI handling

- **Paging** - Enhanced virtual memory
  - Dynamic page table creation
  - Map/unmap functions
  - TLB flush utilities

### Fixed

- Missing ISR handlers for exceptions 9-31
- VGA scrolling issues at screen bottom
- Backspace behavior in shell
- Timer frequency calculation

### Technical Details

- **Memory Layout:**
  - Kernel at 0x100000 (1MB)
  - Heap at 0x400000 (4MB)
  - Page tables at 0x9C000

- **Interrupts:**
  - Full IDT with 256 entries
  - All 32 exception handlers
  - All 16 IRQs
  - System call gate (INT 0x80)

---

## [0.2.0] - Previous

### Added
- First-time username setup
- Persistent configuration (sector 2)
- Basic shell with ls, cat, echo, clear, exit
- Animated ASCII logo at boot

---

## [0.1.0] - Initial

### Added
- Multiboot-compliant bootloader support
- Protected mode (32-bit) operation
- Basic VGA text output
- PS/2 keyboard input
- Simple shell prompt

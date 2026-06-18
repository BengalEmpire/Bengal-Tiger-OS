# Changelog

All notable changes to Bengal Tiger OS will be documented in this file.


## [0.5.0] - 2026-06-18

### Added

#### 🎨 VBE Framebuffer Graphics

- **VBE Linear Framebuffer** — High-resolution graphics via VESA BIOS Extensions, initialized from Multiboot framebuffer info.
  - `vbe.h/c` — `vbe_init()`, `vbe_is_active()`

- **Pixel Drawing Primitives** — Full set of pixel and shape drawing functions with bounds-checking:
  - `vbe_putpixel()` — Draw a single pixel
  - `vbe_getpixel()` — Read pixel color
  - `vbe_fill_rect()` — Fill rectangle with solid color
  - `vbe_draw_rect()` — Draw rectangle outline with configurable thickness
  - `vbe_draw_hline()` / `vbe_draw_vline()` — Draw horizontal/vertical lines

- **Screen Management**:
  - `vbe_clear()` / `vbe_clear_color()` — Clear screen with default or custom color
  - `vbe_scroll()` — Scroll screen up by pixel lines (exposed area filled with background)

- **Text Rendering with Bitmap Font**:
  - `vbe_draw_char()` — Render single 8x16 character pixel-by-pixel
  - `vbe_draw_string()` — Render string with word wrap and newline handling
  - `vbe_puts()` — Render string with default colors
  - `font.h/c` — Standard VGA ROM 8x16 bitmap font, 95 printable characters (ASCII 32-126)

- **Color Constants** — 16 VGA colors pre-mapped to 32-bit RGBA:
  - `VBE_BLACK` through `VBE_WHITE` (16 color constants)
  - `vbe_palette[16]` — Lookup table for VGA → 32-bit conversion
  - `vbe_vga_to_rgb()` — Convert VGA color index (0-15)
  - `vbe_rgb()` — Inline helper to pack R,G,B components

- **Framebuffer Memory Mapping** — VBE init maps the framebuffer physical pages (which may be above 3GB on real hardware) into the kernel's page tables via `paging_map()`.

#### 🖥️ GRUB Graphics Mode Support

- **Multiple Resolution Options** in GRUB menu:
  - `gfxpayload=1024x768x32` — 1024×768 32-bit true color
  - `gfxpayload=800x600x32` — 800×600 32-bit true color
  - `gfxpayload=1280x1024x32` — 1280×1024 32-bit true color
  - `gfxpayload=text` — Traditional VGA text mode (fallback)

- **Graceful Fallback** — If no framebuffer is available (text mode boot), VBE init returns 0 and the kernel operates normally in VGA text mode.

#### 🐚 New Shell Commands

| Command | Description |
|---------|-------------|
| `vbe` | Show framebuffer info (resolution, bpp, pitch, color layout, address, size) + draw demo pattern (16 color bars, cyan border, welcome text) |

- **neofetch** — Now shows `Graphics: 1024x768x32 VBE` when VBE is active

### Changed

- **Kernel version** updated from `0.4.0` to `0.5.0`
- **Boot order** — VBE framebuffer initialized in Phase 3 (after paging, before serial/timer)
- **Shell help** — Updated with `vbe` command listing
- **makefile** — Added `build/vbe.o` and `build/font.o` (24 total object files)

### New Files

| File | Description |
|------|-------------|
| `kernel/font.h` | 8x16 bitmap font header (FONT_WIDTH, FONT_HEIGHT, FONT_NUM_CHARS) |
| `kernel/font.c` | 8x16 bitmap font data (95 characters × 16 bytes) |
| `kernel/vbe.h` | VBE framebuffer types (vbe_fb_t), color defines, drawing API |
| `kernel/vbe.c` | VBE init, pixel/rect/text drawing, paging mapping |

### Technical Details

- **Supported Modes:** 1024×768×32, 800×600×32, 1280×1024×32 (any mode GRUB can set)
- **Pixel Formats:** 24-bit (3 bytes/pixel) and 32-bit (4 bytes/pixel, alpha ignored)
- **Framebuffer Pitch:** Width × bytes_per_pixel, padded to alignment as needed
- **Font Size:** 8×16 pixels, stored as 1 bit per pixel (1520 bytes total)
- **Memory:** Framebuffer is identity-mapped after paging is enabled
- **QEMU:** Requires `-vga std` for VBE support (`make run` includes this flag)
- **Real Hardware:** Requires a VBE-compatible GPU with the requested resolution


## [0.4.0] - 2026-06-18

### Added

#### 🖥️ Real Hardware Boot Stability

- **A20 Gate Enable** — Enables access to memory above 1MB using fast gate (port 0x92) with keyboard controller fallback. Verifies via memory wrap-around test.
  - `cpu.h/c` — `a20_enable()`, `a20_enable_fast()`, `a20_enable_keyboard()`, `a20_check()`

- **Own GDT (Global Descriptor Table)** — No longer depends on GRUB's temporary GDT. Creates a proper flat 32-bit model with:
  - Null descriptor (required by CPU)
  - Kernel Code segment (Ring 0, 32-bit, base 0, limit 4GB)
  - Kernel Data segment (Ring 0, 32-bit, base 0, limit 4GB)
  - Far jump to reload CS after GDT load
  - `gdt.h/c` — `gdt_init()`, `gdt_load()`

- **Stack Initialization** — `boot.s` now sets `ESP = 0x9BFFF` before calling `kmain()`, ensuring a known-good stack regardless of GRUB's state.

- **CPUID Detection + CPU Features** — Detects CPU vendor, brand string, family/model/stepping, and feature flags.
  - `cpu.h/c` — `cpu_detect_cpuid()`, `cpu_query_features()`, `cpu_get_vendor()`, `cpu_get_brand()`

- **FPU Initialization** — Configures CR0 for x87 FPU operation, issues `FNINIT`.
  - `cpu.h/c` — `fpu_init()`

- **SSE/SSE2 Support** — Enables SSE/SSE2 via CR4 OSFXSR/OSXMMEXCPT bits when CPU supports it.
  - `cpu.h/c` — `sse_enable()`

- **Full Multiboot Memory Map Parsing** — Instead of just reading `mem_upper`, parses the full E820 memory map from GRUB for accurate physical memory detection.
  - `multiboot.h` — Full multiboot info structure, `multiboot_mmap_entry_t`, `parse_memory_map()`

- **Dynamic Page Table Allocation** — Page directory and first page table are now allocated right after the kernel's BSS section instead of being hardcoded at `0x9C000`. Prevents conflicts with GRUB modules, ACPI tables, and BIOS data.
  - `paging.c` — Updated `paging_install()`

#### 🕒 Real-Time Clock (RTC) Driver

- **CMOS RTC** — Reads current date and time from the Motorola MC146818 RTC chip.
  - Handles Update-In-Progress (UIP) flag
  - BCD/binary conversion
  - 12-hour/24-hour format support
  - Century register parsing for year 2000+
  - `rtc.h/c` — `rtc_init()`, `rtc_read_time()`, `rtc_format_time()`, `rtc_format_date()`, `rtc_format_datetime()`

#### 🔧 Serial Port Driver

- **16550 UART** — Full serial driver for COM1-COM4 with:
  - Configurable baud rate (default 115200)
  - FIFO enable with 14-byte threshold
  - Automatic LF→CR+LF conversion
  - `serial.h/c` — `serial_init()`, `serial_write_char()`, `serial_write_str()`, `serial_write_hex()`, `serial_write_int()`

#### 💾 Enhanced ATA/ATAPI Disk Driver

- **IDENTIFY Command** — Reads drive model, serial number, firmware revision, sector count
- **LBA48 Support** — 48-bit LBA addressing for drives > 128GB
- **ATAPI Detection** — Detects CD/DVD drives and packet interfaces
- **Error Recovery** — Retry logic with soft reset on failure (up to 3 retries)
- **Soft Reset** — SRST mechanism for channel reset
- `disk.h/c` — `disk_init()`, `ata_identify()`, `ata_get_drive_info()`, `ata_get_capacity()`, `ata_soft_reset()`

#### 🐚 New Shell Commands

| Command | Description |
|---------|-------------|
| `date` | Show real date and time from RTC |
| `disk` | Display ATA drive model, serial, firmware, capacity |
| `cpu` | Show CPU vendor, brand, and feature flags |

#### 🔄 Enhanced Control Commands

- **reboot** — Now tries keyboard controller reset + ACPI + triple fault fallback
- **shutdown** — Now tries QEMU + VirtualBox + Bochs ACPI methods before halting

#### 📋 Updated 7-Phase Boot Sequence

```
Phase 1: CPU Init          (A20 → GDT → CPUID/FPU/SSE)
Phase 2: Critical Hardware (PIC → IDT)
Phase 3: Memory Mgmt       (PMM → Paging → Heap)
Phase 4: Device Drivers    (Serial → Timer → Keyboard → RTC → ATA → FAT → Sched → PCI → NIC)
Phase 5: Enable Interrupts (STI)
Phase 6: User Setup        (Config load → Boot animation → Shell)
Phase 7: Idle Loop         (HLT)
```

### Changed

- **Kernel version** updated from `0.3.0` to `0.4.0`
- **Init order** completely restructured — A20 and GDT now come before PIC/IDT
- **`common.h`** — Added `outl()` and `inl()` 32-bit I/O functions
- **`pci.h`** — Removed duplicate `outl()`/`inl()` (now in `common.h`)
- **`makefile`** — Added new source files: `cpu.o`, `gdt.o`, `rtc.o`, `serial.o`
- **`boot.s`** — Added `mov $0x9BFFF, %esp` for explicit stack setup
- **Shell** — `boot_log()` now outputs to both serial and VGA

### Fixed

- Hardcoded page table address `0x9C000` could overlap with GRUB modules
- `disk_init()` was never called — IDENTIFY data not populated (v0.3.0 regression in new code)
- Unused `f_ebx` variable in `cpu_query_features()` would cause compiler warning
- GDT inline assembly correctly clobbers both `"eax"` and `"memory"`
- Missing CPU exception handlers now properly handled
- ACPI shutdown now supports multiple virtual machine types

### New Files

| File | Description |
|------|-------------|
| `kernel/multiboot.h` | Multiboot info structure definitions |
| `kernel/gdt.h` | GDT header |
| `kernel/gdt.c` | GDT setup and segment reload |
| `kernel/cpu.h` | CPU detection header |
| `kernel/cpu.c` | A20, CPUID, FPU, SSE implementation |
| `kernel/rtc.h` | RTC driver header |
| `kernel/rtc.c` | CMOS RTC implementation |
| `kernel/serial.h` | Serial port header |
| `kernel/serial.c` | 16550 UART implementation |

### Technical Details

- **Memory Layout (updated):**
  - Kernel at `0x100000` (1MB)
  - Page tables dynamically after kernel BSS
  - Heap at `0x400000` (4MB)

- **Boot initialization order (important for real hardware):**
  1. A20 gate (memory access above 1MB)
  2. GDT (own segment descriptors)
  3. CPUID/FPU/SSE (CPU features)
  4. PIC (interrupt routing)
  5. IDT (interrupt handlers)
  6. Memory management
  7. Drivers
  8. Interrupts enabled
  9. User shell

- **Serial debug output available at COM1, 115200 baud, 8N1**

---

## [0.3.0] - 2025-12-26

### Added

#### Kernel Core
- **Kernel Heap Allocator** — Dynamic memory allocation with `kmalloc()`, `kfree()`, `krealloc()`, `kzalloc()`
  - First-fit free-list algorithm
  - Block coalescing for defragmentation
  - Magic number corruption detection
  - Memory statistics tracking

- **Timer Subsystem** — Accurate timing services
  - PIT configured at 100Hz (10ms tick)
  - `sleep_ms()` and `sleep_s()` functions
  - Uptime tracking in ticks, seconds, and formatted string
  - Decoupled from scheduler module

- **Kernel Panic Handler** — Informative crash handling
  - Red panic screen with error message
  - Full CPU register dump
  - Page fault address display (CR2)
  - `ASSERT()` and `PANIC()` macros

- **PCI Bus Scanner** — Hardware detection
  - Full bus/device/function enumeration
  - Vendor and class name lookup
  - Device ID, BAR, and IRQ tracking
  - `pci` shell command to list devices

#### Shell Enhancements
- **Command History** — Navigate with Up/Down arrow keys
- **Cursor Movement** — Left/Right/Home/End key support
- **Delete Key** — Delete character at cursor
- **New Commands:**
  - `neofetch` — System info with ASCII art
  - `uptime` — Show system uptime
  - `mem` — Memory usage statistics
  - `pci` — List PCI devices
  - `history` — Show command history
  - `color <n>` — Change text color (0-15)
  - `reboot` — Reboot the system
  - `shutdown` — Halt the system

#### Keyboard Driver
- **Shift Key Support** — Uppercase letters and symbols
- **Caps Lock** — Toggle case for letters
- **Ctrl/Alt Detection** — Modifier key tracking
- **Special Keys** — Arrow keys, Home, End, Delete
- **Ring Buffer** — Asynchronous key input

#### Documentation
- **README.md** — Comprehensive usage guide
- **docs/ARCHITECTURE.md** — System design documentation
- **docs/BUILD.md** — Build system guide with troubleshooting
- **docs/API.md** — Complete API reference
- **docs/ROADMAP.md** — Future development plans

### Changed

- **Makefile** — Complete rewrite
  - Better organization
  - New targets: `debug`, `help`
  - Improved compiler flags
  - Debug symbol support

- **Shell Prompt** — Now colorful with username highlighting
  - Green username
  - Cyan hostname
  - White separators

- **Boot Sequence** — Added boot animation
  - ASCII tiger art
  - Progress messages with status

- **common.h** — Additional utilities
  - `memcmp()`, `strcpy()`, `strncpy()`, `strcat()`, `strchr()`
  - `MIN()`, `MAX()`, `ALIGN_UP()`, `ALIGN_DOWN()` macros
  - `UNUSED()` macro
  - Optimized `memset()` and `memcpy()`

- **PIC Driver** — Enhanced interrupt controller handling
  - IRQ masking functions
  - Spurious IRQ detection
  - Proper EOI handling

- **Paging** — Enhanced virtual memory
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
  - Page tables at 0x9C000 (hardcoded — fixed in 0.4.0)

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

# Bengal Tiger OS - Architecture Documentation

## Table of Contents
1. [System Overview](#system-overview)
2. [Boot Process](#boot-process)
3. [CPU Initialization](#cpu-initialization)
4. [Memory Management](#memory-management)
5. [Interrupt Handling](#interrupt-handling)
6. [Device Drivers](#device-drivers)
7. [Shell Subsystem](#shell-subsystem)

---

## System Overview

Bengal Tiger OS is a 32-bit protected mode operating system designed for x86 processors. It follows a monolithic kernel architecture where all kernel services run in Ring 0 (kernel mode). The OS is engineered for real hardware compatibility with proper CPU initialization, GDT setup, memory management, and device drivers.

### Design Principles
1. **Real Hardware First** — All code designed to work on real x86 machines, not just emulators
2. **Simplicity** — Clear, readable code over premature optimization
3. **Modularity** — Each subsystem is in separate files with clean interfaces
4. **Diagnostics** — Dual output (serial + VGA) for debugging

### Address Space Layout

```
┌──────────────────────────────────────────┐ 0xFFFFFFFF
│                                          │
│        Unmapped (3GB - 4GB)              │
│                                          │
├──────────────────────────────────────────┤ 0xC0000000
│                                          │
│        Reserved for Future Use           │
│                                          │
├──────────────────────────────────────────┤ 0x01000000 (16MB)
│                                          │
│        Heap Expansion Area               │
│        (grows upward)                    │
│                                          │
├──────────────────────────────────────────┤ 0x00500000 (5MB)
│                                          │
│        Kernel Heap (1MB initial)         │
│                                          │
├──────────────────────────────────────────┤ 0x00400000 (4MB)
│                                          │
│        Kernel BSS + Data                 │
│        Page Tables (dynamic, after BSS)  │
│                                          │
├──────────────────────────────────────────┤ 0x00200000 (2MB)
│                                          │
│        Kernel Code (.text)               │
│                                          │
├──────────────────────────────────────────┤ 0x00100000 (1MB)
│        Extended BIOS Data Area           │
├──────────────────────────────────────────┤ 0x000A0000
│        VGA Buffer (0xB8000)              │
├──────────────────────────────────────────┤ 0x0009F000
│        Stack (temporary, before paging)  │
├──────────────────────────────────────────┤ 0x0009BFFF
│        Conventional Memory               │
├──────────────────────────────────────────┤ 0x00000500
│        BIOS Data Area                    │
├──────────────────────────────────────────┤ 0x00000000
```

---

## Boot Process

### Stage 1: GRUB Bootloader
1. BIOS loads GRUB from MBR
2. GRUB reads `grub.cfg`
3. GRUB loads `kernel.bin` to memory at 1MB
4. GRUB passes multiboot info in EBX
5. GRUB jumps to kernel entry point

### Stage 2: boot.s
```nasm
; Called by GRUB with:
; EBX = pointer to multiboot info structure
start:
    mov $0x9BFFF, %esp     ; Set up stack immediately
    push %ebx              ; Save multiboot info
    call kmain             ; Jump to C code
    cli                    ; Should never reach here
    hlt
1:
    jmp 1b
```

The `mov $0x9BFFF, %esp` is **critical** — it sets up a known-good stack in conventional memory, rather than relying on whatever stack GRUB left for us. Different GRUB versions and BIOS implementations may leave the stack pointer at unpredictable locations.

### Stage 3: kmain() — 7-Phase Initialization

```
Phase 1: CPU Initialization
├── A20 Gate Enable
│   ├── Check if already enabled
│   ├── Fast A20 (port 0x92) — modern method
│   └── Keyboard controller (0x64/0x60) — fallback
└── Global Descriptor Table (GDT)
│   ├── Null descriptor (required)
│   ├── Kernel Code: Ring 0, 32-bit, base 0, limit 4GB
│   ├── Kernel Data: Ring 0, 32-bit, base 0, limit 4GB
│   └── Far jump to reload CS via ljmp
└── CPU Feature Detection
    ├── CPUID availability check (EFLAGS ID bit)
    ├── Vendor string (GenuineIntel, AuthenticAMD, etc.)
    ├── Brand string (full CPU name)
    ├── Feature flags (FPU, SSE, SSE2, MSR, APIC, etc.)
    ├── x87 FPU initialization (CR0 EM/MP, FNINIT)
    └── SSE/SSE2 enable (CR4 OSFXSR/OSXMMEXCPT)

Phase 2: Critical Hardware
├── PIC Remap (8259A)
│   └── IRQ 0-15 → INT 32-47 (avoids CPU exception conflicts)
└── IDT Install
    └── 256 interrupt gates configured

Phase 3: Memory Management
├── Memory Map Parsing
│   ├── Full E820 map from multiboot
│   ├── Falls back to mem_upper if no map
│   └── Only counts MULTIBOOT_MMAP_AVAILABLE regions
├── PMM Init (Physical Memory Manager)
│   └── Bitmap-based page allocator with accurate memory size
├── Paging Enable
│   ├── Identity map first 4MB
│   ├── Page tables allocated AFTER kernel BSS (not hardcoded)
│   └── CR3 loaded with dynamic page directory address
└── Heap Init
    └── Free-list allocator at 4MB

Phase 4: Device Drivers
├── Serial Port (COM1, 115200 baud)
├── Timer (PIT at 100Hz)
├── Keyboard (PS/2)
├── RTC (CMOS clock)
├── ATA Disk (IDENTIFY + PIO read/write)
├── FAT (stub filesystem)
├── Scheduler (single task)
├── PCI Scanner
└── NIC (stub)

Phase 5: Enable Interrupts
└── STI instruction

Phase 6: User Setup
├── Load config.cfg from disk
├── If first boot: prompt for username
└── Initialize shell

Phase 7: Idle Loop
└── HLT until interrupt
```

---

## CPU Initialization

### A20 Gate

The A20 gate controls access to the address line A20. When disabled (as on some real hardware), addresses above 1MB wrap around to the first 1MB. This is **critical** to enable because our kernel loads at 1MB.

```c
int a20_enable(void) {
    // 1. Check if already enabled (memory wrap test)
    // 2. Try fast A20 (port 0x92) - works on most systems
    // 3. Fallback to keyboard controller (more compatible)
}
```

### GDT (Global Descriptor Table)

Bengal Tiger OS installs its own GDT rather than relying on GRUB's. This ensures:

- Known segment descriptors (0x08 = code, 0x10 = data)
- Proper descriptor types and privilege levels
- Independence from bootloader changes

```c
void gdt_init(void) {
    // Null descriptor (index 0)
    // Kernel Code: base=0, limit=4GB, Ring 0, 32-bit (index 1)
    // Kernel Data: base=0, limit=4GB, Ring 0, 32-bit (index 2)
    // Load GDTR via lgdt
    // Reload DS, ES, FS, GS, SS
    // Far jump to reload CS (ljmp $0x08, $flush)
}
```

### CPUID and FPU

```c
int cpu_detect_cpuid(void) {
    // Try to toggle the ID flag (bit 21) in EFLAGS
    // If it toggles, CPUID is available
}

void cpu_query_features(void) {
    // CPUID leaf 0: Vendor string (ebx:edx:ecx)
    // CPUID leaf 1: Family/Model/Stepping + feature flags
    // CPUID leaf 0x80000002-04: Brand string
}

void fpu_init(void) {
    // Clear CR0.EM (Emulation), set CR0.MP (Monitor)
    // Issue FNINIT to reset FPU state
}
```

---

## Memory Management

### Physical Memory Manager (PMM)

The PMM uses a **bitmap allocator** where each bit represents a 4KB page frame. The bitmap size is 32768 × 32 bits = 1,048,576 frames = 4GB max addressable.

```c
static uint32_t bitmap[BITMAP_SIZE];  // 131072 bytes = 128KB bitmap

// Allocation: find first free bit
uint32_t pmm_alloc_frame(void) {
    // Scan bitmap for 0 bit
    // Set bit to 1 (allocated)
    // Return physical address
}

// Deallocation
void pmm_free_frame(uint32_t addr) {
    // Clear bit to 0 (free)
}
```

### Virtual Memory (Paging)

Uses **identity mapping** (virtual = physical) for the first 4MB. The page directory and first page table are **dynamically allocated** right after the kernel's BSS section, avoiding conflicts with GRUB modules, ACPI tables, or BIOS data.

```
Page Directory (1024 entries × 4 bytes = 4KB) — at kernel_end
├── Entry 0 → Page Table 0 (maps 0-4MB)
├── Entry 1 → Not Present
├── Entry 2 → Not Present
│   ...
└── Entry 1023 → Not Present

Page Table 0 (1024 entries × 4 bytes = 4KB) — at kernel_end + 4KB
├── Entry 0 → Physical 0x00000000
├── Entry 1 → Physical 0x00001000
│   ...
└── Entry 1023 → Physical 0x003FF000
```

### Kernel Heap

The heap uses a **first-fit free-list** algorithm with block coalescing, located at 4MB.

```
Block Structure:
┌──────────────────┐
│ Magic: 0xDEADBEEF│ 4 bytes
├──────────────────┤
│ Size (bytes)     │ 4 bytes
├──────────────────┤
│ Is Free (0/1)    │ 1 byte
├──────────────────┤
│ Next Block *     │ 4 bytes
├──────────────────┤
│ Prev Block *     │ 4 bytes
├──────────────────┤
│                  │
│   Data Area      │ N bytes
│                  │
└──────────────────┘

Allocation Strategy:
1. Traverse free list
2. Find first block with size >= requested
3. Split block if significantly larger (>= 16 bytes + header)
4. Mark as allocated
5. Return pointer to data area

Deallocation:
1. Validate magic number (detect corruption)
2. Mark block as free
3. Coalesce with adjacent free blocks (prev and next)
```

---

## Interrupt Handling

### Interrupt Descriptor Table (IDT)

The IDT contains 256 gate descriptors:

| Vector | Type | Description |
|--------|------|-------------|
| 0-31 | Exception | CPU exceptions (divide by zero, page fault, etc.) |
| 32-47 | IRQ | Hardware interrupts (timer, keyboard, disk) |
| 48-127 | Reserved | For future use |
| 128 | System Call | Software interrupt (INT 0x80) |
| 129-255 | Reserved | For future use |

### Exception Handlers

```c
void exception_handler(struct regs *r) {
    // Display panic screen with:
    // - Exception name
    // - Full register dump (EAX, EBX, ECX, EDX, ESI, EDI, EBP, ESP, EIP)
    // - Page fault address (CR2) for #PF
    // Halt system
}
```

| Exception | Vector | Description |
|-----------|--------|-------------|
| #DE | 0 | Divide by Zero |
| #DB | 1 | Debug |
| #NMI | 2 | Non-Maskable Interrupt |
| #BP | 3 | Breakpoint |
| #OF | 4 | Overflow |
| #BR | 5 | Bound Range Exceeded |
| #UD | 6 | Invalid Opcode |
| #NM | 7 | Device Not Available |
| #DF | 8 | Double Fault |
| #TS | 10 | Invalid TSS |
| #NP | 11 | Segment Not Present |
| #SS | 12 | Stack-Segment Fault |
| #GP | 13 | General Protection Fault |
| #PF | 14 | Page Fault |
| #MF | 16 | x87 Floating-Point |
| #AC | 17 | Alignment Check |
| #MC | 18 | Machine Check |
| #XM | 19 | SIMD Floating-Point |

### IRQ Handlers

```
IRQ Flow:
1. Device signals IRQ line
2. PIC asserts INTR pin
3. CPU acknowledges
4. PIC sends vector number
5. CPU invokes ISR stub (isr.s)
6. ISR stub calls irq_handler() (isr.c)
7. Handler dispatches to driver
8. Send EOI to PIC(s)
9. Return from interrupt
```

| IRQ | Vector | Device |
|-----|--------|--------|
| 0 | 32 | PIT Timer (100Hz) |
| 1 | 33 | PS/2 Keyboard |
| 2 | 34 | Cascade (slave PIC) |
| 3 | 35 | COM2 |
| 4 | 36 | COM1 |
| 5 | 37 | LPT2 |
| 6 | 38 | Floppy Disk |
| 7 | 39 | LPT1 / Spurious |
| 8 | 40 | CMOS RTC |
| 12 | 44 | PS/2 Mouse |
| 14 | 46 | Primary ATA |
| 15 | 47 | Secondary ATA |

---

## Device Drivers

### Timer (PIT - Programmable Interval Timer)

```c
// PIT Configuration
#define PIT_FREQUENCY   1193180  // Base frequency
#define TIMER_HZ        100      // Target: 100Hz = 10ms tick
#define TIMER_DIVISOR   (PIT_FREQUENCY / TIMER_HZ)

void timer_init(void) {
    // Configure Channel 0 for 100Hz square wave
    outb(0x43, 0x36);  // Command: Channel 0, lo/hi, mode 3
    outb(0x40, TIMER_DIVISOR & 0xFF);
    outb(0x40, TIMER_DIVISOR >> 8);
}

void timer_handler(struct regs *r) {
    timer_state.ticks++;
    // Update seconds counter every 100 ticks
}
```

### Keyboard (PS/2)

```c
// Scancode Set 1 to ASCII conversion
static const char scancode_lower[128] = {...};
static const char scancode_upper[128] = {...};

void keyboard_handler(void) {
    uint8_t scancode = inb(0x60);

    // Handle modifier keys (Shift, Ctrl, Alt, Caps)
    // Convert to ASCII based on modifiers
    // Send to shell handler
}
```

### Serial Port (16550 UART)

```c
void serial_init(uint16_t port, uint32_t baud) {
    // Disable interrupts
    // Set DLAB to access divisor latch
    // Set baud rate divisor (115200 / baud)
    // 8-bit data, no parity, 1 stop bit (8N1)
    // Enable FIFO with 14-byte threshold
    // Enable DTR, RTS, OUT2
}
```

Debug output: Connect to COM1 at 115200 baud, 8N1 to see kernel boot logs.

### RTC (Real-Time Clock)

```c
// CMOS RTC registers via ports 0x70/0x71
#define CMOS_SECONDS    0x00
#define CMOS_MINUTES    0x02
#define CMOS_HOURS      0x04
#define CMOS_DAY        0x07
#define CMOS_MONTH      0x08
#define CMOS_YEAR       0x09
#define CMOS_CENTURY    0x32   // ACPI century register

int rtc_read_time(rtc_time_t *time) {
    // Wait for Update-In-Progress to clear
    // Read all time registers atomically
    // Convert BCD to binary if needed
    // Handle 12-hour/24-hour format
    // Calculate full year (with century support)
}
```

### ATA Disk Driver

```c
int ata_identify(ata_drive_t *drive, uint8_t channel, uint8_t is_master) {
    // Select drive (master/slave)
    // Send IDENTIFY command (0xEC)
    // If device returns ERR + ABORT → try IDENTIFY PACKET DEVICE (ATAPI)
    // Read 256 words of identification data
    // Parse model string (words 27-46, byte-swapped)
    // Parse serial number (words 10-19)
    // Parse firmware revision (words 23-26)
    // Check LBA48 support (word 83, bit 10)
    // Read sector counts (words 60-61 for 28-bit, 100-103 for 48-bit)
}
```

### PCI Bus

```c
// PCI Configuration Space access via I/O ports
uint32_t pci_config_read(bus, device, func, offset) {
    // Build 32-bit address
    address = (bus << 16) | (device << 11) | (func << 8) | 
              (offset & 0xFC) | 0x80000000;
    
    outl(0xCF8, address);  // Write address
    return inl(0xCFC);     // Read data
}

void pci_init(void) {
    // Scan all 256 buses
    // Scan all 32 devices per bus
    // Check all 8 functions per device
    // Store found devices
}
```

---

## Shell Subsystem

### Command Processing Flow

```
User Types Command
        │
        ▼
keyboard_handler() in keyboard.c
        │
        ▼
shell_handler(char) in shell.c
        │
        ├─── Buffer character in shell_cmd_buf
        │
        └─── On Enter key:
                │
                ▼
        shell_execute_command(cmd)
                │
                ▼
        Parse command name and args
                │
                ▼
        Dispatch to cmd_xxx() function
                │
                ▼
        Print output, show prompt
```

### Command History

```c
static char cmd_history[16][256];  // 16 commands max
static int history_count = 0;
static int history_index = -1;

// Up Arrow: Load previous command
if (history_index == -1) 
    history_index = history_count - 1;
else if (history_index > 0)
    history_index--;
    
memcpy(shell_cmd_buf, cmd_history[history_index], 256);
```

### VGA Text Mode Output

```c
// VGA buffer at 0xB8000, 80x25 characters
volatile uint16_t *video = (uint16_t*)0xB8000;

// Each cell: low byte = character, high byte = attribute
// Attribute: low 4 bits = foreground, high 4 bits = background
video[offset] = character | (color << 8);
```

### Available Commands

| Category | Commands |
|----------|----------|
| **File** | `ls`, `cat <file>` |
| **System** | `info`, `neofetch`, `uptime`, `mem`, `pci`, `date`, `disk`, `cpu` |
| **Shell** | `help`, `echo`, `clear`, `color`, `history` |
| **Control** | `reboot`, `shutdown`, `exit` |

---

## API Reference

### common.h
```c
void outb(uint16_t port, uint8_t val);
void outw(uint16_t port, uint16_t val);
void outl(uint16_t port, uint32_t val);
uint8_t inb(uint16_t port);
uint16_t inw(uint16_t port);
uint32_t inl(uint16_t port);
void memset(void *dest, uint8_t val, uint32_t len);
void memcpy(void *dest, const void *src, uint32_t len);
int memcmp(const void *s1, const void *s2, uint32_t n);
int strlen(const char *s);
int strcmp(const char *s1, const char *s2);
char *strcpy(char *dest, const char *src);
```

### gdt.h
```c
void gdt_init(void);
```

### cpu.h
```c
void cpu_init(void);
int a20_enable(void);
int cpu_detect_cpuid(void);
void cpu_query_features(void);
void fpu_init(void);
int sse_enable(void);
const char* cpu_get_vendor(void);
const char* cpu_get_brand(void);
```

### heap.h
```c
void heap_init(void);
void *kmalloc(uint32_t size);
void *kzalloc(uint32_t size);
void kfree(void *ptr);
void *krealloc(void *ptr, uint32_t new_size);
void heap_get_stats(heap_stats_t *stats);
int heap_check(void);
```

### timer.h
```c
void timer_init(void);
uint32_t timer_get_ticks(void);
uint32_t timer_get_seconds(void);
void sleep_ms(uint32_t ms);
void sleep_s(uint32_t seconds);
void timer_format_uptime(char *buffer);
```

### rtc.h
```c
void rtc_init(void);
int rtc_read_time(rtc_time_t *time);
void rtc_format_time(const rtc_time_t *time, char *buffer);
void rtc_format_date(const rtc_time_t *time, char *buffer);
void rtc_format_datetime(const rtc_time_t *time, char *buffer);
int rtc_is_present(void);
```

### serial.h
```c
void serial_init(uint16_t port, uint32_t baud);
void serial_write_char(uint16_t port, char c);
void serial_write_str(const char *str);
void serial_write_hex(uint16_t port, uint32_t val);
void serial_write_int(uint16_t port, int32_t val);
```

### shell.h
```c
void shell_init(const char *username);
void shell_handler(char ch);
void shell_handler_special(int key);
void shell_print(const char *s);
void shell_print_color(const char *s, uint8_t color);
void shell_clear_screen(void);
void shell_boot_animation(void);
void shell_execute_command(const char *cmd);
```

### pci.h
```c
void pci_init(void);
uint32_t pci_get_device_count(void);
pci_device_t* pci_get_device(uint32_t index);
pci_device_t* pci_find_device(uint16_t vendor, uint16_t device);
const char* pci_vendor_name(uint16_t vendor_id);
const char* pci_class_name(uint8_t class_code);
```

### disk.h
```c
void disk_init(void);
int ata_disk_present(void);
int ata_read_sector(uint32_t lba, uint8_t *buf);
int ata_write_sector(uint32_t lba, uint8_t *buf);
int ata_read_sectors(uint32_t lba, uint32_t count, uint8_t *buf);
int ata_identify(ata_drive_t *drive, uint8_t channel, uint8_t is_master);
ata_drive_t* ata_get_drive_info(void);
uint64_t ata_get_capacity(void);
```

### paging.h
```c
void pmm_init(uint32_t mem_size, uint32_t kernel_end);
uint32_t pmm_alloc_frame(void);
void pmm_free_frame(uint32_t addr);
void paging_install(uint32_t mem_size);
void paging_map(uint32_t virt, uint32_t phys, uint32_t flags);
void paging_unmap(uint32_t virt);
uint32_t paging_get_physical(uint32_t virt);
```

### panic.h
```c
void kernel_panic(const char *message);
void kernel_panic_at(const char *message, const char *file, int line);
#define ASSERT(condition) ...
#define PANIC(msg) ...
```

---

*This document is part of Bengal Tiger OS v0.4.0*

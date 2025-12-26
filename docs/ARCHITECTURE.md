# Bengal Tiger OS - Architecture Documentation

## Table of Contents
1. [System Overview](#system-overview)
2. [Boot Process](#boot-process)
3. [Memory Management](#memory-management)
4. [Interrupt Handling](#interrupt-handling)
5. [Device Drivers](#device-drivers)
6. [Shell Subsystem](#shell-subsystem)

---

## System Overview

Bengal Tiger OS is a 32-bit protected mode operating system designed for x86 processors. It follows a monolithic kernel architecture where all kernel services run in Ring 0 (kernel mode).

### Design Principles
1. **Simplicity** - Clear, readable code over premature optimization
2. **Modularity** - Each subsystem is in separate files with clean interfaces
3. **Documentation** - Every function and module is documented
4. **Learning** - Designed to teach OS development concepts

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
│        Page Directory (0x9C000)          │
├──────────────────────────────────────────┤ 0x0009C000
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
4. GRUB jumps to kernel entry point

### Stage 2: boot.s
```nasm
; Called by GRUB with:
; EBX = pointer to multiboot info structure
start:
    push %ebx        ; Save multiboot info
    call kmain       ; Jump to C code
1:  
    cli              ; Should never reach here
    hlt
    jmp 1b
```

### Stage 3: kmain() Initialization

```
Phase 1: Critical Hardware
├── PIC Remap (8259A)
│   └── IRQ 0-15 → INT 32-47
└── IDT Install
    └── 256 interrupt gates configured

Phase 2: Memory Management
├── PMM Init (Physical Memory Manager)
│   └── Bitmap-based page allocator
├── Paging Enable
│   └── Identity map first 4MB
└── Heap Init
    └── Free-list allocator at 4MB

Phase 3: Device Drivers
├── Timer (PIT at 100Hz)
├── Keyboard (PS/2)
├── Disk (ATA PIO)
├── FAT (stub)
├── PCI Scanner
└── NIC (stub)

Phase 4: Enable Interrupts
└── STI instruction

Phase 5: User Setup
├── Load config.cfg
├── If first boot:
│   └── Prompt for username
└── Initialize shell

Phase 6: Idle Loop
└── HLT until interrupt
```

---

## Memory Management

### Physical Memory Manager (PMM)

The PMM uses a **bitmap allocator** where each bit represents a 4KB page frame.

```c
// Bitmap array: 32768 uint32_t = 1M pages = 4GB max
static uint32_t bitmap[BITMAP_SIZE];

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

Currently using **identity mapping** (virtual = physical) for the first 4MB.

```
Page Directory (1024 entries × 4 bytes = 4KB)
├── Entry 0 → Page Table 0 (maps 0-4MB)
├── Entry 1 → Not Present
├── Entry 2 → Not Present
│   ...
└── Entry 1023 → Not Present

Page Table 0 (1024 entries × 4 bytes = 4KB)
├── Entry 0 → Physical 0x00000000
├── Entry 1 → Physical 0x00001000
│   ...
└── Entry 1023 → Physical 0x003FF000
```

### Kernel Heap

The heap uses a **first-fit free-list** algorithm with block coalescing.

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
3. Split block if significantly larger
4. Mark as allocated
5. Return pointer to data area

Deallocation:
1. Validate magic number
2. Mark block as free
3. Coalesce with adjacent free blocks
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
    // Display panic screen
    // Show register dump
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

---

## API Reference

### common.h
```c
void memset(void *dest, uint8_t val, uint32_t len);
void memcpy(void *dest, const void *src, uint32_t len);
int strlen(const char *s);
int strcmp(const char *s1, const char *s2);
void outb(uint16_t port, uint8_t val);
uint8_t inb(uint16_t port);
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

### shell.h
```c
void shell_init(const char *username);
void shell_handler(char ch);
void shell_print(const char *s);
void shell_print_color(const char *s, uint8_t color);
void shell_clear_screen(void);
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

---

*This document is part of Bengal Tiger OS v0.3.0*

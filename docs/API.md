# Bengal Tiger OS - API Reference

## Overview

This document provides detailed API documentation for all public functions and data structures in Bengal Tiger OS v0.4.0.

---

## Table of Contents

1. [Common Utilities (common.h)](#common-utilities)
2. [GDT (gdt.h)](#gdt)
3. [CPU (cpu.h)](#cpu)
4. [Heap Allocator (heap.h)](#heap-allocator)
5. [Timer (timer.h)](#timer)
6. [RTC (rtc.h)](#rtc)
7. [Serial (serial.h)](#serial)
8. [Keyboard (keyboard.h)](#keyboard)
9. [Disk (disk.h)](#disk)
10. [Shell (shell.h)](#shell)
11. [PCI (pci.h)](#pci)
12. [Paging (paging.h)](#paging)
13. [Panic (panic.h)](#panic)

---

## Common Utilities

**Header:** `kernel/common.h`

### Types

```c
typedef unsigned char       uint8_t;    // 0 to 255
typedef signed char         int8_t;     // -128 to 127
typedef unsigned short      uint16_t;   // 0 to 65535
typedef signed short        int16_t;    // -32768 to 32767
typedef unsigned int        uint32_t;   // 0 to 4294967295
typedef signed int          int32_t;    // -2147483648 to 2147483647
typedef unsigned long long  uint64_t;   // 64-bit unsigned
typedef signed long long    int64_t;    // 64-bit signed
typedef unsigned int        size_t;     // Size type
```

### I/O Functions

#### outb
```c
void outb(uint16_t port, uint8_t val);
```
Write a byte to an I/O port.

**Parameters:**
- `port` - I/O port number (0-65535)
- `val` - Byte value to write (0-255)

**Example:**
```c
outb(0x3D4, 0x0E);  // Write to VGA register
```

---

#### outw
```c
void outw(uint16_t port, uint16_t val);
```
Write a 16-bit word to an I/O port.

---

#### outl
```c
void outl(uint16_t port, uint32_t val);
```
Write a 32-bit double-word to an I/O port.

**Used for:** PCI configuration space access (ports 0xCF8/0xCFC).

---

#### inb
```c
uint8_t inb(uint16_t port);
```
Read a byte from an I/O port.

**Returns:** Byte value read from port

**Example:**
```c
uint8_t scancode = inb(0x60);  // Read keyboard scancode
```

---

#### inw
```c
uint16_t inw(uint16_t port);
```
Read a 16-bit word from an I/O port.

---

#### inl
```c
uint32_t inl(uint16_t port);
```
Read a 32-bit double-word from an I/O port.

---

### Memory Functions

#### memset
```c
void memset(void *dest, uint8_t val, uint32_t len);
```
Fill memory region with a byte value. Optimized for 32-bit aligned writes.

**Parameters:**
- `dest` - Pointer to memory region
- `val` - Byte value to fill with
- `len` - Number of bytes to fill

**Example:**
```c
char buffer[512];
memset(buffer, 0, sizeof(buffer));  // Zero out buffer
```

---

#### memcpy
```c
void memcpy(void *dest, const void *src, uint32_t len);
```
Copy memory from source to destination. Handles overlapping regions safely (backwards copy if dest > src).

**Parameters:**
- `dest` - Destination pointer
- `src` - Source pointer
- `len` - Number of bytes to copy

---

#### memcmp
```c
int memcmp(const void *s1, const void *s2, uint32_t n);
```
Compare two memory regions.

**Returns:**
- `0` if regions are equal
- `<0` if s1 < s2
- `>0` if s1 > s2

---

### String Functions

#### strlen
```c
int strlen(const char *s);
```
Get length of null-terminated string.

**Returns:** Number of characters (excluding null terminator)

---

#### strcmp
```c
int strcmp(const char *s1, const char *s2);
```
Compare two strings.

**Returns:**
- `0` if equal
- `<0` if s1 < s2 lexicographically
- `>0` if s1 > s2

**Example:**
```c
if (strcmp(command, "help") == 0) {
    show_help();
}
```

---

#### strncmp
```c
int strncmp(const char *s1, const char *s2, uint32_t n);
```
Compare up to n characters of two strings.

---

#### strcpy
```c
char *strcpy(char *dest, const char *src);
```
Copy string from src to dest.

**Returns:** Pointer to dest

---

#### strncpy
```c
char *strncpy(char *dest, const char *src, uint32_t n);
```
Copy up to n characters. Pads with null bytes if src is shorter.

---

### Macros

```c
#define NULL ((void*)0)
#define true  1
#define false 0

#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define MAX(a, b) ((a) > (b) ? (a) : (b))

#define ALIGN_UP(val, align)   (((val) + (align) - 1) & ~((align) - 1))
#define ALIGN_DOWN(val, align) ((val) & ~((align) - 1))

#define UNUSED(x) (void)(x)
```

---

## GDT

**Header:** `kernel/gdt.h`

Provides a flat 32-bit protected mode memory model independent of GRUB's GDT.

### Segment Selectors

```c
#define GDT_NULL_SEG     0x00  /* Null descriptor (required) */
#define GDT_KERNEL_CODE  0x08  /* Kernel code segment (Ring 0) */
#define GDT_KERNEL_DATA  0x10  /* Kernel data segment (Ring 0) */
```

### Types

#### gdt_entry_t
```c
typedef struct {
    uint16_t limit_low;     /* Lower 16 bits of segment limit */
    uint16_t base_low;      /* Lower 16 bits of base address */
    uint8_t  base_mid;      /* Next 8 bits of base address */
    uint8_t  access;        /* Access flags (present, ring, type) */
    uint8_t  granularity;   /* Granularity flags + upper 4 bits of limit */
    uint8_t  base_high;     /* Upper 8 bits of base address */
} __attribute__((packed)) gdt_entry_t;
```

### Functions

#### gdt_init
```c
void gdt_init(void);
```
Initialize and load the Global Descriptor Table. Creates three entries (null, code, data), loads GDTR via `lgdt`, reloads all data segment registers, and performs a far jump to reload CS. Must be called early in kmain() before any interrupt or paging initialization.

---

## CPU

**Header:** `kernel/cpu.h`

### Global State

```c
extern cpu_info_t cpu_info;
```

#### cpu_info_t
```c
typedef struct {
    char vendor[13];            // CPU vendor (e.g., "GenuineIntel")
    char brand[49];             // CPU brand string
    uint32_t family;            // CPU family
    uint32_t model;             // CPU model
    uint32_t stepping;          // CPU stepping
    uint32_t features_ecx;      // ECX feature flags
    uint32_t features_edx;      // EDX feature flags
    uint8_t has_cpuid : 1;      // CPUID available
    uint8_t has_fpu   : 1;      // x87 FPU present
    uint8_t has_sse   : 1;      // SSE supported
    uint8_t has_sse2  : 1;      // SSE2 supported
    uint8_t has_msr   : 1;      // RDMSR/WRMSR supported
    uint8_t has_apic  : 1;      // Local APIC present
} cpu_info_t;
```

### Functions

#### cpu_init
```c
void cpu_init(void);
```
Full CPU initialization: enables A20 gate, detects CPUID, queries features, initializes FPU, enables SSE/SSE2.

#### a20_enable
```c
int a20_enable(void);
```
Enable the A20 gate. Tries fast gate (port 0x92) first, then keyboard controller method. Verifies with memory wrap test.

**Returns:** 1 if A20 enabled, 0 on failure

#### cpu_get_vendor
```c
const char* cpu_get_vendor(void);
```
**Returns:** CPU vendor string (e.g., "GenuineIntel", "AuthenticAMD")

#### cpu_get_brand
```c
const char* cpu_get_brand(void);
```
**Returns:** Full CPU brand string (e.g., "Intel(R) Core(TM) i7-8700K CPU @ 3.70GHz")

---

## Heap Allocator

**Header:** `kernel/heap.h`

### Configuration

```c
#define HEAP_START          0x00400000  // 4MB
#define HEAP_INITIAL_SIZE   0x00100000  // 1MB
#define HEAP_MAX_SIZE       0x01000000  // 16MB
```

### Types

#### heap_stats_t
```c
typedef struct {
    uint32_t total_size;        // Total heap size in bytes
    uint32_t used_size;         // Currently allocated bytes
    uint32_t free_size;         // Currently free bytes
    uint32_t num_allocations;   // Number of active allocations
    uint32_t num_blocks;        // Total number of blocks
} heap_stats_t;
```

### Functions

#### heap_init
```c
void heap_init(void);
```
Initialize the kernel heap. Must be called before any kmalloc().

---

#### kmalloc
```c
void *kmalloc(uint32_t size);
```
Allocate memory from the kernel heap.

**Parameters:**
- `size` - Number of bytes to allocate

**Returns:** Pointer to allocated memory, or NULL if failed

**Example:**
```c
int *array = (int *)kmalloc(100 * sizeof(int));
if (array == NULL) {
    // Handle allocation failure
}
```

---

#### kzalloc
```c
void *kzalloc(uint32_t size);
```
Allocate zeroed memory.

**Returns:** Pointer to zeroed memory, or NULL if failed

---

#### kfree
```c
void kfree(void *ptr);
```
Free previously allocated memory.

**Parameters:**
- `ptr` - Pointer from kmalloc (NULL is safe)

**Note:** Double-free is detected via magic number validation and ignored.

---

#### heap_get_stats
```c
void heap_get_stats(heap_stats_t *stats);
```
Get current heap statistics.

**Example:**
```c
heap_stats_t stats;
heap_get_stats(&stats);
printf("Used: %d bytes\n", stats.used_size);
```

---

#### heap_check
```c
int heap_check(void);
```
Verify heap integrity by checking magic numbers in all blocks.

**Returns:**
- `1` if heap is valid
- `0` if corruption detected

---

## Timer

**Header:** `kernel/timer.h`

### Configuration

```c
#define TIMER_HZ 100  // Timer frequency (100Hz = 10ms tick)
```

### Types

#### timer_state_t
```c
typedef struct {
    uint32_t ticks;      // Ticks since boot
    uint32_t seconds;    // Seconds since boot
    uint32_t frequency;  // Timer frequency
} timer_state_t;

extern timer_state_t timer_state;
```

### Functions

#### timer_init
```c
void timer_init(void);
```
Initialize the PIT timer at 100Hz.

---

#### timer_get_ticks
```c
uint32_t timer_get_ticks(void);
```
Get total ticks since boot.

---

#### timer_get_seconds
```c
uint32_t timer_get_seconds(void);
```
Get total seconds since boot.

---

#### sleep_ms
```c
void sleep_ms(uint32_t ms);
```
Sleep for specified milliseconds. Uses HLT for power savings.

**Example:**
```c
sleep_ms(1000);  // Sleep for 1 second
```

---

#### sleep_s
```c
void sleep_s(uint32_t seconds);
```
Sleep for specified seconds.

---

#### timer_format_uptime
```c
void timer_format_uptime(char *buffer);
```
Format uptime as "HH:MM:SS" or "Nd HH:MM:SS" string.

**Parameters:**
- `buffer` - Output buffer (at least 32 bytes)

---

## RTC

**Header:** `kernel/rtc.h`

Provides access to the CMOS Real-Time Clock for reading current date and time.

### Types

#### rtc_time_t
```c
typedef struct {
    uint8_t seconds;     // 0-59
    uint8_t minutes;     // 0-59
    uint8_t hours;       // 0-23
    uint8_t day;         // 1-31
    uint8_t month;       // 1-12
    uint16_t year;       // Full year (e.g., 2026)
    uint8_t weekday;     // 1=Sunday, 7=Saturday
} rtc_time_t;
```

### Functions

#### rtc_init
```c
void rtc_init(void);
```
Initialize RTC driver. Detects RTC presence, determines BCD/binary mode, enables 24-hour format.

---

#### rtc_read_time
```c
int rtc_read_time(rtc_time_t *time);
```
Read the current time from CMOS RTC. Handles Update-In-Progress (UIP) flag for atomic reads, converts BCD to binary, and handles 12-hour/24-hour format.

**Returns:** 1 on success, 0 on failure

---

#### rtc_format_time
```c
void rtc_format_time(const rtc_time_t *time, char *buffer);
```
Format time as "HH:MM:SS". Buffer must be at least 9 bytes.

---

#### rtc_format_date
```c
void rtc_format_date(const rtc_time_t *time, char *buffer);
```
Format date as "YYYY-MM-DD". Buffer must be at least 11 bytes.

---

#### rtc_format_datetime
```c
void rtc_format_datetime(const rtc_time_t *time, char *buffer);
```
Format complete date and time as "YYYY-MM-DD HH:MM:SS". Buffer must be at least 21 bytes.

---

## Serial

**Header:** `kernel/serial.h`

Provides communication via the 16550 UART (serial port). Used primarily for kernel debug output on real hardware.

### Port Constants

```c
#define COM1_PORT   0x3F8
#define COM2_PORT   0x2F8
#define COM3_PORT   0x3E8
#define COM4_PORT   0x2E8
```

### Functions

#### serial_init
```c
void serial_init(uint16_t port, uint32_t baud);
```
Initialize a serial port.

**Parameters:**
- `port` - Base I/O address (e.g., COM1_PORT = 0x3F8)
- `baud` - Baud rate (e.g., 9600, 19200, 38400, 57600, 115200)

Configured as 8 data bits, no parity, 1 stop bit (8N1) with FIFO enabled (14-byte threshold).

---

#### serial_write_char
```c
void serial_write_char(uint16_t port, char c);
```
Write a single character. Automatically expands LF (0x0A) to CR+LF (0x0D 0x0A). Blocks until transmitter is ready.

---

#### serial_write_str
```c
void serial_write_str(const char *str);
```
Write a null-terminated string to the default serial port (COM1). Convenience wrapper for quick debug output.

---

#### serial_write_hex
```c
void serial_write_hex(uint16_t port, uint32_t val);
```
Write a 32-bit value as hex (e.g., "0xDEADBEEF") to the serial port.

---

#### serial_write_int
```c
void serial_write_int(uint16_t port, int32_t val);
```
Write a signed integer in decimal to the serial port.

---

## Keyboard

**Header:** `kernel/keyboard.h`

### Types

#### keyboard_modifiers_t
```c
typedef struct {
    uint8_t shift    : 1;
    uint8_t ctrl     : 1;
    uint8_t alt      : 1;
    uint8_t caps     : 1;
    uint8_t num      : 1;
    uint8_t scroll   : 1;
    uint8_t reserved : 2;
} keyboard_modifiers_t;

extern keyboard_modifiers_t kb_modifiers;
```

#### keycode_t
```c
typedef struct {
    char ascii;              // ASCII character (0 if special)
    uint8_t scancode;        // Raw scancode
    keyboard_modifiers_t mods;  // Modifier state
} keycode_t;
```

### Functions

#### keyboard_init
```c
void keyboard_init(void);
```
Initialize the PS/2 keyboard driver.

---

#### keyboard_has_key
```c
int keyboard_has_key(void);
```
Check if a key is available in the buffer.

---

#### keyboard_getchar
```c
char keyboard_getchar(void);
```
Get next character (blocking). Waits until a key is pressed.

---

#### keyboard_getchar_nonblock
```c
char keyboard_getchar_nonblock(void);
```
Get next character (non-blocking, returns 0 if none).

---

#### keyboard_is_shift_pressed
```c
int keyboard_is_shift_pressed(void);
int keyboard_is_ctrl_pressed(void);
int keyboard_is_alt_pressed(void);
int keyboard_is_caps_on(void);
```
Check modifier key states.

---

## Disk

**Header:** `kernel/disk.h`

Enhanced ATA/ATAPI PIO-mode disk driver with IDENTIFY command support.

### Constants

```c
#define SECTOR_SIZE     512
#define ATA_MAX_RETRIES 3
#define ATA_TIMEOUT     1000000
```

### Types

#### ata_drive_t
```c
typedef struct {
    uint8_t  drive_type;         // ATA_TYPE_ATA or ATA_TYPE_ATAPI
    uint8_t  channel;            // 0 = primary, 1 = secondary
    uint8_t  is_master;          // 1 = master, 0 = slave
    uint8_t  is_lba48;           // Supports 48-bit LBA
    uint32_t sectors_28;         // Sectors (28-bit LBA)
    uint64_t sectors_48;         // Sectors (48-bit LBA)
    char     model[41];          // Model string
    char     serial[21];         // Serial number
    char     firmware[9];        // Firmware revision
    uint16_t pio_mode;           // PIO mode supported
    uint16_t dma_mode;           // DMA mode supported
    uint8_t  present;            // 1 if drive present
} ata_drive_t;
```

### Functions

#### disk_init
```c
void disk_init(void);
```
Initialize ATA subsystem. Scans primary master and runs the IDENTIFY command. Must be called before any read/write operations.

---

#### ata_disk_present
```c
int ata_disk_present(void);
```
Check if an ATA drive is present.

**Returns:** 1 if present, 0 if not

---

#### ata_read_sector
```c
int ata_read_sector(uint32_t lba, uint8_t *buf);
```
Read a single sector (512 bytes) from disk. Includes automatic retry on error (up to 3 retries with soft reset).

**Parameters:**
- `lba` - Logical Block Address
- `buf` - Buffer (must be at least 512 bytes)

**Returns:** 0 on success, -1 on error

---

#### ata_write_sector
```c
int ata_write_sector(uint32_t lba, uint8_t *buf);
```
Write a single sector to disk. Flushes write cache after completion.

---

#### ata_read_sectors
```c
int ata_read_sectors(uint32_t lba, uint32_t count, uint8_t *buf);
```
Read multiple consecutive sectors.

---

#### ata_get_drive_info
```c
ata_drive_t* ata_get_drive_info(void);
```
Get pointer to the detected drive information structure.

**Returns:** Pointer to ata_drive_t, or NULL if no drive detected

---

#### ata_get_capacity
```c
uint64_t ata_get_capacity(void);
```
Get drive capacity in bytes.

**Returns:** Capacity in bytes, or 0 if no drive

---

## Shell

**Header:** `kernel/shell.h`

### Constants

```c
#define VGA_COLOR_BLACK         0
#define VGA_COLOR_BLUE          1
#define VGA_COLOR_GREEN         2
#define VGA_COLOR_CYAN          3
#define VGA_COLOR_RED           4
#define VGA_COLOR_MAGENTA       5
#define VGA_COLOR_BROWN         6
#define VGA_COLOR_LIGHT_GREY    7
#define VGA_COLOR_DARK_GREY     8
#define VGA_COLOR_LIGHT_BLUE    9
#define VGA_COLOR_LIGHT_GREEN   10
#define VGA_COLOR_LIGHT_CYAN    11
#define VGA_COLOR_LIGHT_RED     12
#define VGA_COLOR_LIGHT_MAGENTA 13
#define VGA_COLOR_YELLOW        14
#define VGA_COLOR_WHITE         15
```

### Functions

#### shell_init
```c
void shell_init(const char *username);
```
Initialize shell with username. Clears screen, shows welcome message and prompt.

---

#### shell_handler
```c
void shell_handler(char ch);
```
Handle keyboard character input. Buffers characters, executes on Enter.

---

#### shell_print
```c
void shell_print(const char *s);
```
Print string with default color.

---

#### shell_print_color
```c
void shell_print_color(const char *s, uint8_t color);
```
Print string with specific color.

**Color format:** `foreground | (background << 4)`

**Example:**
```c
shell_print_color("Error!", 0x4F);  // White on red
```

---

#### shell_print_int
```c
void shell_print_int(int num);
```
Print integer.

---

#### shell_print_hex
```c
void shell_print_hex(uint32_t num);
```
Print hexadecimal value (with 0x prefix).

---

#### shell_clear_screen
```c
void shell_clear_screen(void);
```
Clear the screen.

---

## PCI

**Header:** `kernel/pci.h`

### Types

#### pci_device_t
```c
typedef struct {
    uint8_t bus;
    uint8_t device;
    uint8_t function;
    uint16_t vendor_id;
    uint16_t device_id;
    uint8_t class_code;
    uint8_t subclass;
    uint8_t prog_if;
    uint8_t revision;
    uint8_t header_type;
    uint8_t irq;
    uint32_t bar[6];      // Base Address Registers
} pci_device_t;
```

### Functions

#### pci_init
```c
void pci_init(void);
```
Scan all 256 PCI buses, 32 devices per bus, 8 functions per device.

---

#### pci_get_device_count
```c
uint32_t pci_get_device_count(void);
```
Returns number of detected PCI devices.

---

#### pci_get_device
```c
pci_device_t* pci_get_device(uint32_t index);
```
Get device by index (0 to count-1).

---

#### pci_find_device
```c
pci_device_t* pci_find_device(uint16_t vendor_id, uint16_t device_id);
```
Find device by vendor and device ID.

**Example:**
```c
// Find Intel E1000 NIC
pci_device_t *nic = pci_find_device(0x8086, 0x100E);
```

---

#### pci_find_device_by_class
```c
pci_device_t* pci_find_device_by_class(uint8_t class_code, uint8_t subclass);
```
Find device by class/subclass.

**Common classes:**
- `0x01` - Mass Storage
- `0x02` - Network
- `0x03` - Display
- `0x06` - Bridge

---

#### pci_class_name
```c
const char* pci_class_name(uint8_t class_code);
```
Get human-readable class name string.

---

## Paging

**Header:** `kernel/paging.h`

### Configuration

```c
#define PAGE_SIZE 4096  // 4KB pages
```

### Functions

#### pmm_init
```c
void pmm_init(uint32_t mem_size, uint32_t kernel_end);
```
Initialize physical memory manager with a bitmap allocator.

---

#### pmm_alloc_frame
```c
uint32_t pmm_alloc_frame(void);
```
Allocate a 4KB physical page frame.

**Returns:** Physical address, or 0 if out of memory

---

#### pmm_free_frame
```c
void pmm_free_frame(uint32_t addr);
```
Free a physical page frame.

---

#### paging_install
```c
void paging_install(uint32_t mem_size);
```
Enable paging with identity-mapped first 4MB. Page directory and first page table are dynamically allocated right after the kernel's BSS section.

---

#### paging_map
```c
void paging_map(uint32_t virt, uint32_t phys, uint32_t flags);
```
Map a virtual address to a physical address with specified flags.

---

#### paging_get_physical
```c
uint32_t paging_get_physical(uint32_t virt);
```
Translate virtual address to physical. Returns 0 if not mapped.

---

## Panic

**Header:** `kernel/panic.h`

### Functions

#### kernel_panic
```c
void kernel_panic(const char *message);
```
Halt system with error message. Displays red panic screen. Never returns.

---

#### kernel_panic_at
```c
void kernel_panic_at(const char *message, const char *file, int line);
```
Panic with file/line information.

---

### Macros

```c
// Assert condition, panic if false
#define ASSERT(condition) \
    do { \
        if (!(condition)) { \
            kernel_panic_at("Assertion failed: " #condition, __FILE__, __LINE__); \
        } \
    } while (0)

// Panic with auto file/line
#define PANIC(msg) kernel_panic_at(msg, __FILE__, __LINE__)
```

**Example:**
```c
void *ptr = kmalloc(1024);
ASSERT(ptr != NULL);

if (error_condition) {
    PANIC("Something went wrong!");
}
```

---

*This document is part of Bengal Tiger OS v0.4.0*

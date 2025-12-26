# Bengal Tiger OS - API Reference

## Overview

This document provides detailed API documentation for all public functions and data structures in Bengal Tiger OS.

---

## Table of Contents

1. [Common Utilities (common.h)](#common-utilities)
2. [Heap Allocator (heap.h)](#heap-allocator)
3. [Timer (timer.h)](#timer)
4. [Keyboard (keyboard.h)](#keyboard)
5. [Shell (shell.h)](#shell)
6. [PCI (pci.h)](#pci)
7. [Paging (paging.h)](#paging)
8. [Panic (panic.h)](#panic)

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

### Memory Functions

#### memset
```c
void memset(void *dest, uint8_t val, uint32_t len);
```
Fill memory region with a byte value.

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
Copy memory from source to destination. Handles overlapping regions safely.

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

#### kmalloc_aligned
```c
void *kmalloc_aligned(uint32_t size, uint32_t alignment);
```
Allocate memory with specific alignment.

**Parameters:**
- `size` - Number of bytes
- `alignment` - Alignment requirement (must be power of 2)

---

#### kfree
```c
void kfree(void *ptr);
```
Free previously allocated memory.

**Parameters:**
- `ptr` - Pointer from kmalloc (NULL is safe)

**Note:** Double-free is detected and ignored.

---

#### krealloc
```c
void *krealloc(void *ptr, uint32_t new_size);
```
Resize an allocation.

**Special cases:**
- `ptr == NULL` → behaves like kmalloc(new_size)
- `new_size == 0` → behaves like kfree(ptr)

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
Verify heap integrity.

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

#### timer_get_ms
```c
uint32_t timer_get_ms(void);
```
Get total milliseconds since boot (may wrap).

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
Get next character (blocking).

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

### Global Variables

```c
extern char shell_username[32];    // Current username
extern int shell_setup_mode;       // 1 during first-time setup
```

### Functions

#### shell_init
```c
void shell_init(const char *username);
```
Initialize shell with username.

---

#### shell_handler
```c
void shell_handler(char ch);
```
Handle keyboard character input.

---

#### shell_handler_special
```c
void shell_handler_special(int key);
```
Handle special keys (arrows, home, end).

**Key codes:**
- `SPECIAL_KEY_UP` (0x100)
- `SPECIAL_KEY_DOWN` (0x101)
- `SPECIAL_KEY_LEFT` (0x102)
- `SPECIAL_KEY_RIGHT` (0x103)
- `SPECIAL_KEY_HOME` (0x104)
- `SPECIAL_KEY_END` (0x105)
- `SPECIAL_KEY_DELETE` (0x106)

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
Scan PCI bus and detect all devices.

---

#### pci_get_device_count
```c
uint32_t pci_get_device_count(void);
```
Get number of detected PCI devices.

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

#### pci_config_read
```c
uint32_t pci_config_read(uint8_t bus, uint8_t device, uint8_t func, uint8_t offset);
```
Read 32 bits from PCI configuration space.

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
Initialize physical memory manager.

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
Enable paging with identity-mapped first 4MB.

---

## Panic

**Header:** `kernel/panic.h`

### Functions

#### kernel_panic
```c
void kernel_panic(const char *message);
```
Halt system with error message. Never returns.

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

*This document is part of Bengal Tiger OS v0.3.0*

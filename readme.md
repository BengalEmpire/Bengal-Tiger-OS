
# Bengal Tiger OS (Prototype Kernel)

A minimal 32-bit hobby operating system that boots via **GRUB (Multiboot)**, enters **protected mode**, and writes directly to **VGA text memory (0xB8000)**.
This serves as a clean foundation for building a full OS — keyboard input, paging, scheduler, filesystem, and beyond.

---

## Features (Current Prototype)

✔ Boots using **GRUB (Multiboot Specification)**
✔ Enters **32-bit protected mode** and executes `kmain()`
✔ Writes **two static lines** and a **live counter** to VGA text buffer
✔ Fully automated **Makefile build & run process**

---

## Explanation of the OS
Bengal Tiger OS is a minimal 32-bit hobby operating system written from scratch in C and assembly. It's designed as a proof-of-concept for OS development, following the Multiboot specification to boot via GRUB (a common bootloader). The goal is to provide a foundation for features like interrupts, memory management, and user interaction without relying on any external libraries or standard OS components, everything is custom-built.


### How to Start and What the User Sees

1.  **Starting**:
    -   In QEMU: Run the command; GRUB menu appears briefly (timeout 5s), selects "Bengal Tiger (prototype)", loads kernel.
    -   On Hardware: Insert USB/CD, reboot, select in boot menu.
2.  **What the User Sees**:
    -   **Boot Screen**: GRUB menu with one entry. After loading:
        -   Row 1: "Bengal Tiger OS" (light gray on black VGA text).
        -   Row 2: "Hello from kernel (32-bit Multiboot)!".
        -   Row 3: "Count: 0" (starts at 0, increments every ~10ms via timer—looks like a live counter).
        -   Row 4+: Keystrokes appear live as you type (e.g., press 'a' → 'a' shows). Type commands like "help" + Enter → "Commands: help, ls, cat".
        -   For "ls" + Enter: "Files: hello.txt".
        -   For "cat" + Enter: Displays "Hello from file!" (from sample file).
    -   Screen is 80x25 text mode. No graphics—pure console. If no input, just the counter ticks forever.
    -   Example Interaction: Type "help" → See response on screen. Errors or unknown commands: Nothing (stub shell).

### How It Works (High-Level Flow)

1.  **Boot Process**:
    -   GRUB (from the ISO) loads the kernel binary (kernel.bin) into memory at address 0x100000 (1MB).
    -   The assembly bootloader (boot.s) verifies the Multiboot magic, enters 32-bit protected mode (x86 mode with flat memory access), and calls kmain in C, passing memory info.
2.  **Kernel Initialization**:
    -   kmain prints static messages ("Bengal Tiger OS" and "Hello from kernel...") to the VGA text buffer (memory-mapped at 0xB8000).
    -   Initializes PIC (remaps IRQs to avoid conflicts).
    -   Sets up IDT (for exceptions like divide-by-zero and IRQs like keyboard/timer).
    -   Enables paging (virtual memory) and allocates physical frames using a bitmap.
    -   Installs keyboard handler (IRQ1) and timer (IRQ0 for ~100Hz ticks).
    -   Initializes FAT filesystem (reads boot sector via ATA disk reads).
    -   Sets up scheduler (stub for now, but ticks update the counter).
    -   Enables interrupts (sti instruction).
3.  **Runtime Behavior**:
    -   The kernel enters an infinite hlt loop (halt until interrupt), making it interrupt-driven instead of polling.
    -   **Timer IRQ (every ~10ms)**: Updates a counter on screen (e.g., "Count: 123") via scheduler_tick.
    -   **Keyboard IRQ**: Reads scancodes from port 0x60, maps to ASCII, passes to shell. Keys appear live on screen.
    -   **Shell**: Buffers input; on Enter, parses simple commands:
        -   "help": Shows command list.
        -   "ls": Lists files (stub: "Files: hello.txt").
        -   "cat": Loads and displays "hello.txt" (if present in ISO).
    -   No exit—runs forever until power off.
4.  **Technical Details**:
    -   **Mode**: 32-bit protected (no real-mode code after boot).
    -   **No Stdlib**: Custom functions for I/O (inb/outb), string ops (strlen), memory (memset).
    -   **Interrupts**: Handled via assembly stubs (isr.s) that save registers and call C handlers.
    -   **Memory**: Identity-mapped kernel; allocates frames > kernel end.
    -   **Filesystem**: Assumes IDE disk; reads sectors for FAT boot and files (simplified, not full directory traversal).

## Project Structure

The project is organized for modularity, with separate directories for boot code, kernel sources, GRUB configuration, and build artifacts. This structure promotes clean separation of concerns and easier maintenance.

```
bengaltiger-os/
├── Makefile              # Build automation: compiles, links, creates ISO, runs QEMU
├── linker.ld             # Linker script: defines memory layout (e.g., load at 1MiB)
├── boot/
│   └── boot.s            # Assembly: Multiboot header, entry point, calls kmain
├── kernel/
│   ├── main.c            # Kernel entry: kmain() with VGA output, init calls
│   ├── common.c          # Utility functions: outb, inb, memset, strlen, etc.
│   ├── common.h          # Headers for common utilities
│   ├── idt.c             # Interrupt Descriptor Table setup
│   ├── idt.h             # IDT structures and functions
│   ├── isr.s             # Assembly: ISR/IRQ stubs for exceptions and interrupts
│   ├── isr.c             # C handlers for ISRs and IRQs
│   ├── pic.c             # Programmable Interrupt Controller remapping
│   ├── keyboard.c        # Keyboard IRQ handler and scancode mapping
│   ├── keyboard.h        # Keyboard functions
│   ├── paging.c          # Paging setup and physical memory manager (bitmap allocator)
│   ├── paging.h          # Paging and PMM functions
│   ├── disk.c            # Low-level ATA disk read (for filesystem)
│   ├── disk.h            # Disk functions
│   ├── fat.c             # Basic FAT filesystem parser and file loader
│   ├── fat.h             # FAT structures and functions
│   ├── scheduler.c       # Multitasking: task structs, round-robin scheduler
│   ├── scheduler.h       # Scheduler functions
│   ├── shell.c           # CLI shell: command buffer, parsing (e.g., ls, cat)
│   ├── pci.c             # PCI scanning for devices (e.g., NIC)
│   └── nic.c             # Network Interface Card drivers (E1000/RTL8139 stubs)
├── grub/
│   └── grub.cfg          # GRUB config: menuentry for kernel
├── build/                # Temporary build artifacts (objects, kernel.bin)
│   ├── *.o               # Object files
│   └── kernel.bin        # Linked kernel binary
└── iso/                  # ISO staging directory
    └── boot/
        ├── grub/         # GRUB files for ISO
        │   └── grub.cfg
        └── kernel.bin    # Copied kernel
```

- **Note**: The `build/` and `iso/` directories are created dynamically by the Makefile. Add sample files (e.g., `hello.txt`) to `iso/boot/` for filesystem testing.

---

## Code Guidelines and Best Practices

To ensure the OS remains maintainable, scalable, and debuggable, follow these modified guidelines when extending the code:

### General Principles
- **Modularity**: Keep features in separate files (e.g., `idt.c` for IDT, `keyboard.c` for input). Use headers (`.h`) for declarations and `.c` for implementations. Include guards (`#ifndef`) in headers.
- **No Standard Library**: Continue using custom utilities (e.g., `strlen`, `memset` in `common.c`). Avoid `stdio.h` or similar—everything from scratch.
- **32-bit Focus**: All code targets x86 32-bit (`-m32` flags). Plan for 64-bit migration later (Roadmap).
- **Error Handling**: Add basic checks (e.g., null pointers, allocation failures). For now, halt on errors (`cli; hlt;` loop).
- **Comments and Style**: Use descriptive comments. Follow consistent indentation (4 spaces/tabs). Name variables/functions clearly (e.g., `pmm_alloc_frame`).
- **Testing**: Build incrementally—test one feature at a time in QEMU. Use `-d int` for interrupt debugging, `-serial stdio` for logs.
- **Real Hardware Compatibility**: Test on legacy BIOS machines. Avoid assumptions about hardware (e.g., PS/2 keyboard init if needed).
- **Multiboot Usage**: Pass `multiboot_info_t *mbi` to `kmain` for memory map. Use it in `pmm_init` for accurate free memory detection.
- **Interrupt Safety**: Set up IDT/PIC before `sti`. Handle page faults in IDT.

### Specific Guidelines by Feature
- **Keyboard Input**: Map scancodes to ASCII in `keyboard.c`. Handle releases/shifts for full input. Integrate with shell for buffering.
- **Memory Management**: Use bitmap for PMM in `paging.c`. Identity-map kernel. Alloc/free frames safely.
- **Filesystem**: Read sectors via `disk.c`. Parse FAT boot sector/root dir in `fat.c`. Load files by name (e.g., for shell `cat`).
- **Multitasking**: Define task structs in `scheduler.c`. Use timer IRQ for switching. Start with kernel/user tasks.
- **CLI Shell**: Buffer input in `shell.c`. Parse simple commands (e.g., `ls` via FAT, `ps` for tasks).
- **Device Drivers**: Scan PCI in `pci.c`. Stub NIC init—expand to ring buffers/IRQs later.
- **Interrupt-Driven**: Replace polls (e.g., counter) with IRQs. Use `hlt` in main loop.

### Build Modifications
- Update `Makefile` to include new `.c`/`.s` files in `OBJS`.
- Add rules for new objects: e.g., `build/keyboard.o: kernel/keyboard.c $(CC) $(CFLAGS) -c -o $@ $<`
- For assembly: `$(AS) $(ASFLAGS) -o $@ $<`
- Clean: Remove new build artifacts.


---

## Build & Run (Linux)

### 1 Install Build Dependencies (Debian / Ubuntu Example)

```bash
sudo apt update
sudo apt install build-essential gcc-multilib grub-pc-bin grub-common xorriso qemu-system-x86
```

>  `gcc-multilib` enables `-m32`. Without it, you must port this to 64-bit.

### 2 Build the OS

```bash
make all
```

This will compile everything and produce **`bengaltiger.iso`**.

### 3 Run in QEMU (Recommended)

```bash
make run
```

Or manually:

```bash
qemu-system-i386 -cdrom bengaltiger.iso -m 512M
```

### 4 Boot on Real Hardware (⚠ Be Careful!)

You can write the ISO to USB using `dd`:

```bash
sudo dd if=bengaltiger.iso of=/dev/sdX bs=4M status=progress && sync
```

Or use **Rufus / Etcher** on Windows.

---

##  Roadmap / Next Steps

| Feature                   | Description                                                          | Status |
| ------------------------- | -------------------------------------------------------------------- | ------ |
|  Migrate to 64-bit      | UEFI bootloader + Long Mode kernel                                   | Planned |


---


**Author:** *Mahmud Rahman*


---

### Working on:

* A **badge-style header** or **ASCII art logo**
* A **visual screenshot / GIF of boot screen**
* A **GitHub-friendly LICENSE (MIT / BSD / GPL)**

**If you run it and face issues (e.g., QEMU crashes), check logs with qemu ... -d int -serial stdio. This OS is expandable**
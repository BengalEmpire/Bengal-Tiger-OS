# Bengal Tiger OS v1.0.0 - System Architecture

## 1. Boot & Memory Map

```
0x00000000 - 0x000FFFFF : Low Memory (BIOS IVT, BDA, VGA 0xB8000) [Reserved in PMM]
0x00100000 - 0x0018BFFF : Kernel ELF32 Image (.multiboot, .text, .rodata, .data, .bss + 32KB stack)
0x0018C000 - 0x00194FFF : Kernel Page Directory (4KB) + 8 Identity Page Tables (32KB -> maps 0..32MB)
0x00400000 - 0x013FFFFF : Expandable Kernel Heap (2MB initial, dynamically expands up to 16MB)
0x01400000 - 0x01FFFFFF : Identity-Mapped Physical Frame Pool
0xFD000000+ / High MMIO : Dynamically mapped VBE Framebuffer & Intel E1000 MMIO BAR0
```

### Boot Sequence (`boot/boot.s` -> `kernel/main.c:kmain`)
1. **GRUB Multiboot1** loads `build/kernel.bin` at `0x00100000`, sets up the requested `1024x768x32` linear framebuffer (or VGA text mode), and jumps to `start` with `EAX = 0x2BADB002` and `EBX = multiboot_info_t*`.
2. **`boot/boot.s`** disables interrupts (`cli`), clears direction flag (`cld`), establishes the 16-byte-aligned 32 KB `.bss` bootstrap stack, and calls `kmain(mboot, magic)`.
3. **Phase 0**: `serial_init(COM1, 115200)` and `klog_init()` bring up instant UART telemetry and the 8 KB `dmesg` ring buffer.
4. **Phase 1**: `cpu_init()` (non-destructive A20 check/enable, CPUID, FPU, SSE), `gdt_init()`, `pic_remap()`, and `idt_install()` (CPU exceptions `0..31`, hardware IRQs `32..47`, and `INT 0x80` syscall gate).
5. **Phase 2**: `pmm_init_mmap()` parses BIOS E820 regions, `paging_install()` identity-maps `0..32MB` and enables `CR0.PG`, and `heap_init_at()` initializes the interrupt-safe expandable kernel heap.
6. **Phase 3**: `timer_init()` (100 Hz PIT), `rtc_init()` (CMOS UTC clock), and `vbe_init()` (maps VBE framebuffer via `paging_map_region`).
7. **Phase 4**: `keyboard_init()`, `mouse_init()`, `disk_init()`, `fat_init()` (auto-mounts persistent ATA FAT volume or 512 KB Live RAMDisk), `pci_init()`, `net_init()`, and `nic_init()` (Intel E1000 or Realtek RTL8139).
8. **Phase 5**: `scheduler_init()`, `sti()`, `net_autoconfig()` (live DHCPv4 lease), and `shell_init()` -> `shell_run()`.

## 2. Interrupt & Input Decoupling

Hardware interrupt handlers (`IRQ0` timer, `IRQ1` PS/2 keyboard, `IRQ4` COM1 UART, `IRQ12` PS/2 mouse, NIC IRQ) execute quickly with interrupts masked and **never** block or run shell commands inside ISR context:
- `IRQ1` and `IRQ4` push raw scancodes/bytes into lock-protected ring buffers.
- The foreground shell (`shell_run`), text editor (`editor_open`), monitor (`top`), and desktop (`gui`) consume unified events via `input_poll_event()` / `input_wait_event()` in normal thread context (`IF=1`), allowing `hlt()`, `sleep_ms()`, `net_ping()`, and preemptive task switching to run concurrently without deadlock.

## 3. Storage & Network Subsystems

- **Storage (`disk.c` + `fat.c`)**: Scans Primary (`0x1F0`/`0x3F6`) and Secondary (`0x170`/`0x376`) IDE channels across all 4 Master/Slave slots, distinguishing ATAPI CD-ROMs from ATA hard drives. Supports Superfloppy (LBA 0) and MBR-partitioned (`0x1BE`) FAT12/FAT16 volumes, plus an automatic 512 KB Live RAMDisk when no formatted HDD is attached.
- **Networking (`nic.c` + `net.c`)**: Static 16-byte-aligned DMA descriptor rings below 32 MB ensure physical == virtual addresses for PCI Bus Mastering on both Intel E1000 and Realtek RTL8139 controllers, feeding an RFC-compliant Ethernet/ARP/IPv4/ICMP/UDP/DHCPv4 stack.

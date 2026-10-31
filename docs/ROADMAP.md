# Bengal Tiger OS Roadmap

## Completed in v1.0.0 ("Royal Bengal")
- [x] Multiboot1-compliant ELF32 boot & 32 KB aligned kernel stack
- [x] E820 physical frame allocator, 32 MB identity paging + dynamic MMIO page tables, expandable 16 MB heap
- [x] Preemptive round-robin multitasking scheduler & `INT 0x80` syscall gate
- [x] Unified VBE high-resolution console (`1024x768x32`), VGA `80x25` text console & COM1 serial terminal
- [x] Interactive VBE Graphical Desktop Workspace (`gui`) with PS/2 mouse support
- [x] Persistent FAT12/FAT16 filesystem (`mkfs`, MBR + Superfloppy) + automatic 512 KB Live RAMDisk
- [x] Built-in full-screen text editor (`edit`) & shell script interpreter (`sh`)
- [x] Intel E1000 & Realtek RTL8139 PCI Ethernet drivers + ARP, IPv4, UDP, DHCPv4 (`dhcp`) & ICMP (`ping`)
- [x] Automated CI verification (ASan/UBSan host tests + 2-boot QEMU E2E integration tests)

## Future Post-1.0 Milestones
- [ ] Hierarchical FAT subdirectories (`mkdir`, `cd`, `rmdir`) and FAT32 / VFAT long filenames
- [ ] TCP state machine, DNS stub resolver (`nslookup`), and built-in HTTP client (`fetch`)
- [ ] Ring-3 user-mode ELF executable loader with isolated per-process page directories
- [ ] Sound Blaster 16 / AC'97 PCI audio driver
- [ ] Symmetric Multiprocessing (SMP) via Local APIC & I/O APIC

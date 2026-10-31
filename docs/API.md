# Kernel Subsystem API Overview

## Core & Diagnostics
- `kernel/common.h`: Fixed-width types, `outb`/`inb`/`outw`/`inw`/`outl`/`inl`, `irq_save()`/`irq_restore()`, `htons`/`htonl`/`ntohs`/`ntohl`, string/memory helpers, `k_atoi`/`k_itoa`/`k_utoa`.
- `kernel/klog.h`: `klog_init()`, `klog_writeln(subsys, msg)`, `klog_get_buffer(&len)` (8 KB circular log exposed via `dmesg`).
- `kernel/panic.h`: `PANIC(msg)` and `ASSERT(cond)` outputting diagnostics across Serial COM1, VGA text memory, and VBE framebuffer.

## Memory & Multitasking
- `kernel/paging.h`: `pmm_init_mmap(mbi, fallback, reserved_end)`, `pmm_alloc_frame()`, `pmm_free_frame()`, `paging_install()`, `paging_map_region(virt, phys, size, flags)`.
- `kernel/heap.h`: `heap_init_at(base, init_sz, max_sz)`, `kmalloc()`, `kzalloc()`, `kmalloc_aligned()`, `krealloc()`, `kfree()`, `heap_check()`.
- `kernel/scheduler.h`: `scheduler_init()`, `task_create(name, fn)`, `task_kill(pid)`, `scheduler_yield()` (`INT 0x80` `SYS_YIELD = 158`), `scheduler_get_task_list(&count)`.

## Input & Display
- `kernel/keyboard.h` & `kernel/serial.h`: `input_poll_event()`, `input_wait_event()` unifying PS/2 Set-1 scancodes (`0xE0` navigation, `Ctrl+Key`, function keys) with VT100/ANSI serial escape sequences.
- `kernel/mouse.h`: `mouse_init()`, `mouse_get_x()`, `mouse_get_y()`, `mouse_get_buttons()`.
- `kernel/vbe.h`: `vbe_init(...)`, `vbe_putpixel()`, `vbe_fill_rect()`, `vbe_draw_char()`, `vbe_draw_string()`, `vbe_scroll_region()`.

## Storage & Filesystem
- `kernel/disk.h`: `disk_init()`, `ata_read_sector()`, `ata_write_sector()`, `disk_enable_ramdisk()`, `disk_has_hardware_hdd()`.
- `kernel/fat.h`: `fat_init()`, `fat_format(label, use_ata)`, `fat_read_file()`, `fat_write_file()`, `fat_append_file()`, `fat_delete_file()`, `fat_copy_file()`, `fat_rename_file()`, `fat_stat_file()`, `fat_get_fs_info()`.

## PCI & Networking
- `kernel/pci.h`: `pci_init()`, `pci_find_device(vendor, device)`, `pci_config_read()`, `pci_config_write()`.
- `kernel/nic.h`: `nic_init()` (Intel E1000 + Realtek RTL8139), `nic_send_packet()`, `nic_poll()`, `nic_handle_irq()`.
- `kernel/net.h`: `net_init()`, `net_autoconfig()`, `net_dhcp_discover()`, `net_arp_resolve()`, `net_ping()`, `net_send_udp()`.

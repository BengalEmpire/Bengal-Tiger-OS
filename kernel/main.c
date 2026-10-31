/**
 * Bengal Tiger OS - Kernel Entry Point & Boot Orchestrator
 *
 * Initializes all hardware drivers, memory managers, filesystems, networking,
 * and preemptive multitasking in strict dependency order before launching
 * the interactive shell.
 *
 * @file main.c
 * @author Bengal Tiger OS (BengalEmpire)
 * @version 1.0.0
 */

#include "version.h"
#include "multiboot.h"
#include "common.h"
#include "serial.h"
#include "klog.h"
#include "panic.h"
#include "cpu.h"
#include "gdt.h"
#include "idt.h"
#include "pic.h"
#include "paging.h"
#include "heap.h"
#include "timer.h"
#include "rtc.h"
#include "vbe.h"
#include "keyboard.h"
#include "mouse.h"
#include "disk.h"
#include "fat.h"
#include "pci.h"
#include "nic.h"
#include "net.h"
#include "scheduler.h"
#include "shell.h"

extern uint8_t bss_end[];

void kmain(multiboot_info_t *mboot, uint32_t magic) {
    /* Phase 0: Early Serial UART & Kernel Ring Log */
    serial_init(COM1_PORT, 115200);
    klog_init();

    serial_write_str("\n========================================================\n");
    serial_write_str("  " OS_NAME " v" OS_VERSION " (" OS_CODENAME ") Booting...\n");
    serial_write_str("========================================================\n");

    if (magic != MULTIBOOT_BOOTLOADER_MAGIC || !mboot) {
        PANIC("Invalid Multiboot magic number from bootloader");
    }

    /* Phase 1: CPU, GDT, 8259A PIC & IDT (Exceptions + IRQs + INT 0x80) */
    cpu_init();
    klog_writeln("cpu", cpu_get_brand()[0] ? cpu_get_brand() : cpu_get_vendor());

    gdt_init();
    pic_remap();
    pic_unmask_irq(4); /* Enable COM1 serial RX interrupt */
    idt_install();
    klog_writeln("core", "GDT, IDT, INT 0x80 syscalls & 8259A PIC initialized");

    /* Phase 2: E820 Physical Memory Manager, Paging & Dynamic Kernel Heap */
    uint32_t total_mem_bytes = multiboot_total_memory(mboot);
    uint32_t k_end = (uint32_t)(uintptr_t)bss_end;
    uint32_t heap_base = ALIGN_UP(k_end + 0x10000U, 0x100000U);
    if (heap_base < HEAP_START) {
        heap_base = HEAP_START;
    }

    pmm_init_mmap(mboot, total_mem_bytes, heap_base + HEAP_INITIAL_SIZE);
    paging_install(total_mem_bytes);
    heap_init_at((void*)(uintptr_t)heap_base, HEAP_INITIAL_SIZE, HEAP_MAX_SIZE);
    klog_writeln("mm", "E820 PMM, 32MB identity paging & expandable heap online");

    /* Phase 3: PIT System Timer, CMOS RTC & VBE/VGA Display */
    timer_init();
    rtc_init();
    if (vbe_init(mboot->flags,
                 mboot->framebuffer_addr,
                 mboot->framebuffer_pitch,
                 mboot->framebuffer_width,
                 mboot->framebuffer_height,
                 mboot->framebuffer_bpp,
                 mboot->framebuffer_type,
                 mboot->framebuffer_red_field_position,
                 mboot->framebuffer_red_mask_size,
                 mboot->framebuffer_green_field_position,
                 mboot->framebuffer_green_mask_size,
                 mboot->framebuffer_blue_field_position,
                 mboot->framebuffer_blue_mask_size)) {
        klog_writeln("video", "VBE linear framebuffer initialized");
    } else {
        klog_writeln("video", "VGA 80x25 text mode active");
    }

    /* Phase 4: PS/2 Input, ATA/RAMDisk Storage, FAT12/16, PCI & NIC */
    keyboard_init();
    mouse_init();
    klog_writeln("input", "PS/2 keyboard, mouse & COM1 serial input ready");

    disk_init();
    fat_init();

    pci_init();
    net_init();
    nic_init();

    /* Phase 5: Preemptive Scheduler, Interrupts & Network Autoconfig */
    scheduler_init();
    klog_writeln("sched", "Preemptive round-robin task scheduler active");

    sti();

    net_autoconfig();

    /* Launch Interactive Shell */
    shell_init();
    shell_run();
}

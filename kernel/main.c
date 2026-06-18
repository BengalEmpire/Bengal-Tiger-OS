/**
 * Bengal Tiger OS - Kernel Main Entry Point
 * 
 * This is the main kernel initialization code that sets up all
 * hardware, initializes subsystems, and starts the shell.
 * 
 * @file main.c
 * @author Bengal Tiger OS (BengalEmpire)
 * @version 0.4.0
 */

#include "common.h"
#include "multiboot.h"
#include "gdt.h"
#include "cpu.h"
#include "idt.h"
#include "pic.h"
#include "keyboard.h"
#include "paging.h"
#include "disk.h"
#include "fat.h"
#include "scheduler.h"
#include "shell.h"
#include "pci.h"
#include "nic.h"
#include "timer.h"
#include "heap.h"
#include "panic.h"
#include "rtc.h"
#include "serial.h"

/* External symbols from linker */
extern uint32_t bss_end;

/* Kernel Information */
#define KERNEL_NAME     "Bengal Tiger OS"
#define KERNEL_VERSION  "0.4.0"
#define KERNEL_ARCH     "i386"

/* Boot flags */
static int first_boot = 0;
static uint32_t total_memory = 0;

/* Saved multiboot info for memory map parsing */
static multiboot_info_t *saved_mbi = NULL;

/**
 * Print boot progress message
 */
static void boot_log(const char *component, const char *status) {
    serial_write_str("  [");
    serial_write_str(status);
    serial_write_str("] ");
    serial_write_str(component);
    serial_write_str("\n");

    shell_print_color("  [", 0x07);
    if (strcmp(status, "OK") == 0) {
        shell_print_color(status, 0x0A);
    } else if (strcmp(status, "FAIL") == 0) {
        shell_print_color(status, 0x0C);
    } else {
        shell_print_color(status, 0x0E);
    }
    shell_print_color("] ", 0x07);
    shell_print(component);
    shell_print("\n");
}

/**
 * Parse the multiboot memory map and calculate physical memory size.
 */
static uint32_t parse_memory_map(multiboot_info_t *mbi) {
    uint32_t total = 0;

    if (!(mbi->flags & (1 << 6)) || mbi->mmap_length == 0) {
        if (mbi->flags & 0x01) {
            total = (mbi->mem_upper * 1024) + 0x100000;
        } else {
            total = 16 * 1024 * 1024;
        }
        boot_log("Memory (fallback)", "WARN");
        return total;
    }

    multiboot_mmap_entry_t *entry = multiboot_mmap_first(mbi);
    while (multiboot_mmap_has_more(mbi, entry)) {
        if (entry->type == MULTIBOOT_MMAP_AVAILABLE) {
            total += (uint32_t)entry->len;
        }
        entry = multiboot_mmap_next(entry);
    }

    if (total == 0) {
        total = 16 * 1024 * 1024;
    }

    return total;
}

/**
 * Kernel Main Entry Point
 * 
 * Called by boot.s after multiboot bootloader hands off control.
 * 
 * @param mbi Pointer to multiboot information structure
 */
void kmain(multiboot_info_t *mbi) {
    saved_mbi = mbi;

    /* ============================================ */
    /* Phase 1: CPU Initialization                 */
    /* ============================================ */

    if (a20_enable()) {
        boot_log("A20 Gate", "OK");
    } else {
        boot_log("A20 Gate", "FAIL");
    }

    gdt_init();
    boot_log("GDT", "OK");

    cpu_init();
    boot_log("CPU Features", "OK");
    if (cpu_info.has_fpu) {
        boot_log("FPU", "OK");
    }
    if (cpu_info.has_sse || cpu_info.has_sse2) {
        boot_log("SSE/SSE2", "OK");
    }

    /* ============================================ */
    /* Phase 2: Critical Hardware                  */
    /* ============================================ */

    pic_remap();
    boot_log("PIC", "OK");

    idt_install();
    boot_log("IDT", "OK");

    /* ============================================ */
    /* Phase 3: Memory Management                  */
    /* ============================================ */

    total_memory = parse_memory_map(mbi);

    uint32_t kernel_end = (uint32_t)&bss_end;
    pmm_init(total_memory, kernel_end);
    boot_log("PMM", "OK");

    paging_install(total_memory);
    boot_log("Paging", "OK");

    heap_init();
    boot_log("Heap", "OK");
    
    /* ============================================ */
    /* Phase 4: Device Drivers                     */
    /* ============================================ */

    serial_init(COM1_PORT, 115200);
    boot_log("Serial", "OK");

    timer_init();
    boot_log("Timer", "OK");

    keyboard_init();
    boot_log("Keyboard", "OK");

    rtc_init();
    boot_log("RTC", "OK");

    disk_init();
    if (ata_disk_present()) {
        boot_log("ATA Disk", "OK");
    } else {
        boot_log("ATA Disk", "WARN");
    }

    fat_init();
    boot_log("FAT", "OK");

    scheduler_init();
    boot_log("Scheduler", "OK");

    pci_init();
    boot_log("PCI Scan", "OK");

    nic_init();
    boot_log("NIC", "OK");
    
    /* ============================================ */
    /* Phase 5: Enable Interrupts                  */
    /* ============================================ */
    __asm__ volatile("sti");
    boot_log("Interrupts", "OK");

    /* ============================================ */
    /* Phase 6: User Configuration & Setup         */
    /* ============================================ */
    
    uint8_t config[512];
    memset(config, 0, 512);
    fat_load_file("config.cfg", config);
    
    char username[32] = {0};
    
    /* Check if this is first boot (config empty) */
    if (config[0] == 0xFF || config[0] == 0x00) {
        first_boot = 1;
        
        /* Display boot animation */
        shell_boot_animation();
        
        /* First-time setup */
        shell_print_color("\n\nWelcome to Bengal Tiger OS!\n\n", 0x0E);
        shell_print("Please enter your username: ");
        shell_setup_mode = 1;
        
        /* Wait for setup to complete */
        while (shell_setup_mode) {
            __asm__ volatile("hlt");
        }
        
        /* Save configuration */
        memcpy(username, shell_username, 31);
        memset(config, 0, 512);
        memcpy(config, username, strlen(username));
        fat_save_file("config.cfg", config);
        
    } else {
        /* Load existing username */
        memcpy(username, config, 31);
        
        /* Show brief boot animation */
        shell_boot_animation();
        
        /* Initialize shell with loaded username */
        sleep_ms(500);
        shell_init(username);
    }
    
    /* ============================================ */
    /* Phase 7: Main Idle Loop                     */
    /* ============================================ */

    serial_write_str("\n*** Bengal Tiger OS boot complete ***\n");
    serial_write_str("Version: 0.4.0 | Architecture: i386\n");
    serial_write_str("Ready. Waiting for input...\n");

    while (1) {
        __asm__ volatile("hlt");
    }
}

uint32_t get_total_memory(void) {
    return total_memory;
}

int is_first_boot(void) {
    return first_boot;
}
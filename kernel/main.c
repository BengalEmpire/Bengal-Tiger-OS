/**
 * Bengal Tiger OS - Kernel Main Entry Point
 * 
 * This is the main kernel initialization code that sets up all
 * hardware, initializes subsystems, and starts the shell.
 * 
 * @file main.c
 * @author Bengal Tiger OS (BengalEmpire)
 * @version 0.3.0
 */

#include "common.h"
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

/* Multiboot Information Structure (Partial) */
typedef struct {
    uint32_t flags;             /* Multiboot info version number */
    uint32_t mem_lower;         /* Available memory from BIOS (KB) */
    uint32_t mem_upper;         /* Available memory above 1MB (KB) */
    uint32_t boot_device;       /* "root" partition */
    uint32_t cmdline;           /* Kernel command line */
    uint32_t mods_count;        /* Number of modules loaded */
    uint32_t mods_addr;         /* Pointer to modules */
    /* ... more fields follow */
} __attribute__((packed)) multiboot_info_t;

/* External symbols from linker */
extern uint32_t bss_end;

/* Kernel Information */
#define KERNEL_NAME     "Bengal Tiger OS"
#define KERNEL_VERSION  "0.3.0"
#define KERNEL_ARCH     "i386"

/* Boot flags */
static int first_boot = 0;
static uint32_t total_memory = 0;

/**
 * Print boot progress message
 */
static void boot_log(const char *component, const char *status) {
    shell_print_color("  [", 0x07);
    if (strcmp(status, "OK") == 0) {
        shell_print_color(status, 0x0A);  /* Green */
    } else if (strcmp(status, "FAIL") == 0) {
        shell_print_color(status, 0x0C);  /* Red */
    } else {
        shell_print_color(status, 0x0E);  /* Yellow */
    }
    shell_print_color("] ", 0x07);
    shell_print(component);
    shell_print("\n");
}

/**
 * Kernel Main Entry Point
 * 
 * Called by boot.s after multiboot bootloader hands off control.
 * 
 * @param mbi Pointer to multiboot information structure
 */
void kmain(multiboot_info_t *mbi) {
    /* ============================================ */
    /* Phase 1: Critical Hardware Initialization   */
    /* ============================================ */
    
    /* 1.1 - Remap PIC (Programmable Interrupt Controller)
     * Maps IRQ 0-7 to INT 32-39, IRQ 8-15 to INT 40-47
     * This prevents conflicts with CPU exceptions (INT 0-31)
     */
    pic_remap();
    
    /* 1.2 - Install IDT (Interrupt Descriptor Table)
     * Sets up CPU exception handlers and IRQ handlers
     */
    idt_install();
    
    /* ============================================ */
    /* Phase 2: Memory Management                  */
    /* ============================================ */
    
    /* 2.1 - Calculate total available memory */
    if (mbi->flags & 0x01) {
        total_memory = (mbi->mem_upper * 1024) + 0x100000;  /* mem_upper + 1MB */
    } else {
        total_memory = 16 * 1024 * 1024;  /* Assume 16MB if not provided */
    }
    
    /* 2.2 - Initialize Physical Memory Manager
     * Uses bitmap to track physical page allocation
     */
    uint32_t kernel_end = (uint32_t)&bss_end;
    pmm_init(total_memory, kernel_end);
    
    /* 2.3 - Setup Paging (Virtual Memory)
     * Identity maps first 4MB, enables paging
     */
    paging_install(total_memory);
    
    /* 2.4 - Initialize Kernel Heap
     * Provides kmalloc/kfree for dynamic allocation
     */
    heap_init();
    
    /* ============================================ */
    /* Phase 3: Device Drivers                     */
    /* ============================================ */
    
    /* 3.1 - Timer (PIT - Programmable Interval Timer)
     * Configured for 100Hz (10ms interval)
     */
    timer_init();
    
    /* 3.2 - PS/2 Keyboard Driver */
    keyboard_init();
    
    /* 3.3 - ATA Disk Driver (stub) */
    fat_init();
    
    /* 3.4 - Scheduler (stub) */
    scheduler_init();
    
    /* 3.5 - PCI Bus Scan
     * Detects all PCI devices on the bus
     */
    pci_init();
    
    /* 3.6 - Network Driver (stub) */
    nic_init();
    
    /* ============================================ */
    /* Phase 4: Enable Interrupts                  */
    /* ============================================ */
    __asm__ volatile("sti");
    
    /* ============================================ */
    /* Phase 5: Load User Configuration            */
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
    /* Phase 6: Main Idle Loop                     */
    /* ============================================ */
    
    /* 
     * The kernel now enters an idle loop.
     * All work is done via interrupt handlers:
     * - IRQ0 (32): Timer tick for scheduling
     * - IRQ1 (33): Keyboard input handling
     * - Other IRQs: Hardware devices
     * 
     * The HLT instruction puts the CPU into a low-power
     * state until the next interrupt arrives.
     */
    while (1) {
        __asm__ volatile("hlt");
    }
}
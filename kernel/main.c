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

// Multiboot info definition
typedef struct {
    uint32_t flags;
    uint32_t mem_lower;
    uint32_t mem_upper;
} multiboot_info_t;

extern uint32_t bss_end; // Retaining the existing uint32_t type
extern volatile uint32_t tick;

void sleep_ms(uint32_t ms) {
    uint32_t start = tick;
    // Tick is approx 10ms (100Hz)
    while (tick - start < (ms / 10)) {
        __asm__ volatile("hlt");
    }
}

void kmain(multiboot_info_t *mbi) {
    // 1. Initialize Hardware
    pic_remap();
    idt_install();
    
    // 2. Memory Management
    uint32_t mem_size = (mbi->mem_upper * 1024) + 0x100000;
    // Pass the address of the BSS end symbol (resolved by the linker)
    pmm_init(mem_size, (uint32_t)&bss_end); 
    paging_install(mem_size);

    // 3. Drivers
    keyboard_install();
    fat_init();
    scheduler_init();
    pci_init();
    nic_init();

    // 4. Timer (PIT) ~100Hz
    outb(0x43, 0x36);
    outb(0x40, 0xA9); // 1193180 / 100 Hz approx = 11931
    outb(0x40, 0x04); // Actually 0x2E9B for 100Hz

    __asm__ volatile("sti"); // Enable Interrupts

    // 5. Load Config
    uint8_t config[512];
    memset(config, 0, 512);
    // Try to load config from sector 2
    fat_load_file("config.cfg", config);

    char username[32] = {0};
    
    // Check if sector is empty (00 or FF usually)
    if (config[0] == 0xFF || config[0] == 0x00) {
        shell_print("Welcome to Bengal Tiger OS!\n");
        shell_print("First boot setup. Enter username: ");
        shell_setup_mode = 1;
        
        while (shell_setup_mode) {
            __asm__ volatile("hlt");
        }
        
        // Setup done, save config
        memcpy(username, shell_username, 31);
        memset(config, 0, 512);
        memcpy(config, username, strlen(username));
        fat_save_file("config.cfg", config);
    } else {
        memcpy(username, config, 31);
        shell_init(username);
    }

    // 6. Loop
    while (1) {
        __asm__ volatile("hlt");
    }
}
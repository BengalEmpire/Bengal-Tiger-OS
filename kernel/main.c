// kernel/main.c

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

// Multiboot struct (simplified)
typedef struct {
    uint32_t mem_lower;
    uint32_t mem_upper;
    // Add more if needed
} multiboot_info_t;

extern uint32_t bss_end;

// Custom strlen
static int strlen(const char *s) {
    int i = 0;
    while (s[i]) i++;
    return i;
}

// Print to VGA
static void puts_vga(const char *s) {
    volatile uint16_t *video = (volatile uint16_t*)0xB8000;
    int len = strlen(s);
    for (int i = 0; i < len; ++i) {
        uint8_t ch = (uint8_t)s[i];
        uint8_t attr = 0x07;
        video[i] = (uint16_t)ch | ((uint16_t)attr << 8);
    }
}

// Kernel entry
void kmain(multiboot_info_t *mbi) {
    const char *msg1 = "Bengal Tiger OS";
    const char *msg2 = "Hello from kernel (32-bit Multiboot)!";

    puts_vga(msg1);

    volatile uint16_t *video = (volatile uint16_t*)0xB8000;
    int row2_offset = 80;
    int i = 0;
    while (msg2[i]) {
        uint8_t ch = (uint8_t)msg2[i];
        uint8_t attr = 0x07;
        video[row2_offset + i] = (uint16_t)ch | ((uint16_t)attr << 8);
        i++;
    }

    // Inits
    pic_remap();
    idt_install();
    uint32_t mem_size = (mbi->mem_upper * 1024) + 0x100000;
    pmm_init(mem_size, (uint32_t)&bss_end);
    paging_install(mem_size);
    keyboard_install();
    fat_init();
    scheduler_init();
    shell_init();  // If needed
    pci_init();  // Stub
    nic_init();  // Stub

    // PIT timer ~100Hz
    outb(0x43, 0x36);
    outb(0x40, 0xA9);
    outb(0x40, 0x04);

    __asm__ volatile("sti");

    while (1) {
        __asm__ volatile("hlt");
    }
}
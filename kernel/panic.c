/**
 * Bengal Tiger OS - Kernel Panic Handler Implementation
 * 
 * @file panic.c
 * @author Bengal Tiger OS (BengalEmpire)
 * @version 0.3.0
 */

#include "panic.h"
#include "common.h"

/* Exception names for diagnostics */
const char *exception_names[32] = {
    "Division By Zero",
    "Debug",
    "Non Maskable Interrupt",
    "Breakpoint",
    "Into Detected Overflow",
    "Out of Bounds",
    "Invalid Opcode",
    "No Coprocessor",
    "Double Fault",
    "Coprocessor Segment Overrun",
    "Bad TSS",
    "Segment Not Present",
    "Stack Fault",
    "General Protection Fault",
    "Page Fault",
    "Unknown Interrupt",
    "Coprocessor Fault",
    "Alignment Check",
    "Machine Check",
    "Reserved", "Reserved", "Reserved", "Reserved", "Reserved",
    "Reserved", "Reserved", "Reserved", "Reserved", "Reserved",
    "Reserved", "Reserved", "Reserved"
};

/* VGA text mode buffer */
static volatile uint16_t *panic_vga = (volatile uint16_t *)0xB8000;

/* Panic screen colors */
#define PANIC_COLOR_HEADER  0x4F  /* White on red */
#define PANIC_COLOR_TEXT    0x4E  /* Yellow on red */
#define PANIC_COLOR_INFO    0x4C  /* Red on red (dim) */

/* Simple print function for panic screen */
static void panic_print(const char *str, int row, int col, uint8_t color) {
    int offset = row * 80 + col;
    while (*str) {
        if (*str != '\n') {
            panic_vga[offset++] = (uint16_t)*str | ((uint16_t)color << 8);
        }
        str++;
    }
}

/* Print a hex value */
static void panic_print_hex(uint32_t value, int row, int col, uint8_t color) {
    char hex[11] = "0x00000000";
    const char *hex_chars = "0123456789ABCDEF";
    
    for (int i = 9; i >= 2; i--) {
        hex[i] = hex_chars[value & 0xF];
        value >>= 4;
    }
    
    panic_print(hex, row, col, color);
}

/* Fill screen with panic background */
static void panic_clear_screen(void) {
    for (int i = 0; i < 80 * 25; i++) {
        panic_vga[i] = ' ' | (PANIC_COLOR_TEXT << 8);
    }
}

void kernel_panic(const char *message) {
    /* Disable interrupts immediately */
    __asm__ volatile("cli");
    
    panic_clear_screen();
    
    /* Draw panic header */
    panic_print("================================================================================", 0, 0, PANIC_COLOR_HEADER);
    panic_print("                         BENGAL TIGER OS - KERNEL PANIC                        ", 1, 0, PANIC_COLOR_HEADER);
    panic_print("================================================================================", 2, 0, PANIC_COLOR_HEADER);
    
    /* Error message */
    panic_print("FATAL ERROR:", 4, 2, PANIC_COLOR_TEXT);
    panic_print(message, 5, 4, PANIC_COLOR_TEXT);
    
    /* Tiger ASCII art */
    panic_print("        ,'/", 8, 50, PANIC_COLOR_INFO);
    panic_print("       / /", 9, 50, PANIC_COLOR_INFO);
    panic_print("      / /", 10, 50, PANIC_COLOR_INFO);
    panic_print("  ,-`-.`-.", 11, 50, PANIC_COLOR_INFO);
    panic_print(" /  ___ \\", 12, 50, PANIC_COLOR_INFO);
    panic_print(" `-.___.-'", 13, 50, PANIC_COLOR_INFO);
    
    /* Instructions */
    panic_print("The system has been halted to prevent damage.", 16, 2, PANIC_COLOR_TEXT);
    panic_print("Please restart your computer.", 17, 2, PANIC_COLOR_TEXT);
    
    panic_print("--------------------------------------------------------------------------------", 19, 0, PANIC_COLOR_HEADER);
    panic_print("Press the reset button or power cycle to restart.", 20, 2, PANIC_COLOR_INFO);
    panic_print("--------------------------------------------------------------------------------", 21, 0, PANIC_COLOR_HEADER);
    
    /* Halt forever */
    for (;;) {
        __asm__ volatile("hlt");
    }
}

void kernel_panic_at(const char *message, const char *file, int line) {
    __asm__ volatile("cli");
    
    panic_clear_screen();
    
    /* Draw panic header */
    panic_print("================================================================================", 0, 0, PANIC_COLOR_HEADER);
    panic_print("                         BENGAL TIGER OS - KERNEL PANIC                        ", 1, 0, PANIC_COLOR_HEADER);
    panic_print("================================================================================", 2, 0, PANIC_COLOR_HEADER);
    
    /* Error details */
    panic_print("FATAL ERROR:", 4, 2, PANIC_COLOR_TEXT);
    panic_print(message, 5, 4, PANIC_COLOR_TEXT);
    
    panic_print("Location:", 7, 2, PANIC_COLOR_TEXT);
    panic_print("  File: ", 8, 2, PANIC_COLOR_INFO);
    panic_print(file, 8, 10, PANIC_COLOR_TEXT);
    panic_print("  Line: ", 9, 2, PANIC_COLOR_INFO);
    
    /* Print line number */
    char line_str[12];
    int pos = 0;
    int temp = line;
    if (temp == 0) {
        line_str[pos++] = '0';
    } else {
        char digits[10];
        int num_digits = 0;
        while (temp > 0) {
            digits[num_digits++] = '0' + (temp % 10);
            temp /= 10;
        }
        while (num_digits > 0) {
            line_str[pos++] = digits[--num_digits];
        }
    }
    line_str[pos] = '\0';
    panic_print(line_str, 9, 10, PANIC_COLOR_TEXT);
    
    /* Instructions */
    panic_print("The system has been halted to prevent damage.", 12, 2, PANIC_COLOR_TEXT);
    panic_print("Please restart your computer.", 13, 2, PANIC_COLOR_TEXT);
    
    /* Halt forever */
    for (;;) {
        __asm__ volatile("hlt");
    }
}

void exception_handler(struct regs *r) {
    if (r->int_no < 32) {
        __asm__ volatile("cli");
        
        panic_clear_screen();
        
        /* Draw panic header */
        panic_print("================================================================================", 0, 0, PANIC_COLOR_HEADER);
        panic_print("                         BENGAL TIGER OS - CPU EXCEPTION                       ", 1, 0, PANIC_COLOR_HEADER);
        panic_print("================================================================================", 2, 0, PANIC_COLOR_HEADER);
        
        /* Exception info */
        panic_print("EXCEPTION:", 4, 2, PANIC_COLOR_TEXT);
        panic_print(exception_names[r->int_no], 4, 14, PANIC_COLOR_TEXT);
        
        /* Register dump */
        panic_print("REGISTER DUMP:", 6, 2, PANIC_COLOR_TEXT);
        
        panic_print("EAX:", 7, 4, PANIC_COLOR_INFO);
        panic_print_hex(r->eax, 7, 9, PANIC_COLOR_TEXT);
        panic_print("EBX:", 7, 22, PANIC_COLOR_INFO);
        panic_print_hex(r->ebx, 7, 27, PANIC_COLOR_TEXT);
        panic_print("ECX:", 7, 40, PANIC_COLOR_INFO);
        panic_print_hex(r->ecx, 7, 45, PANIC_COLOR_TEXT);
        panic_print("EDX:", 7, 58, PANIC_COLOR_INFO);
        panic_print_hex(r->edx, 7, 63, PANIC_COLOR_TEXT);
        
        panic_print("ESI:", 8, 4, PANIC_COLOR_INFO);
        panic_print_hex(r->esi, 8, 9, PANIC_COLOR_TEXT);
        panic_print("EDI:", 8, 22, PANIC_COLOR_INFO);
        panic_print_hex(r->edi, 8, 27, PANIC_COLOR_TEXT);
        panic_print("EBP:", 8, 40, PANIC_COLOR_INFO);
        panic_print_hex(r->ebp, 8, 45, PANIC_COLOR_TEXT);
        panic_print("ESP:", 8, 58, PANIC_COLOR_INFO);
        panic_print_hex(r->esp, 8, 63, PANIC_COLOR_TEXT);
        
        panic_print("EIP:", 9, 4, PANIC_COLOR_INFO);
        panic_print_hex(r->eip, 9, 9, PANIC_COLOR_TEXT);
        panic_print("CS:", 9, 22, PANIC_COLOR_INFO);
        panic_print_hex(r->cs, 9, 27, PANIC_COLOR_TEXT);
        panic_print("EFLAGS:", 9, 40, PANIC_COLOR_INFO);
        panic_print_hex(r->eflags, 9, 48, PANIC_COLOR_TEXT);
        
        if (r->int_no == 14) {
            /* Page fault - show CR2 (faulting address) */
            uint32_t cr2;
            __asm__ volatile("mov %%cr2, %0" : "=r"(cr2));
            panic_print("PAGE FAULT ADDRESS (CR2):", 11, 2, PANIC_COLOR_TEXT);
            panic_print_hex(cr2, 11, 28, PANIC_COLOR_TEXT);
            
            panic_print("Error code:", 12, 2, PANIC_COLOR_TEXT);
            panic_print_hex(r->err_code, 12, 15, PANIC_COLOR_TEXT);
        }
        
        /* Instructions */
        panic_print("The system has been halted to prevent damage.", 15, 2, PANIC_COLOR_TEXT);
        panic_print("Please restart your computer.", 16, 2, PANIC_COLOR_TEXT);
        
        /* Halt forever */
        for (;;) {
            __asm__ volatile("hlt");
        }
    }
}

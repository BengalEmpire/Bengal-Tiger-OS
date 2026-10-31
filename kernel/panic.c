/**
 * Bengal Tiger OS - Kernel Panic & Exception Handler
 *
 * Outputs full crash diagnostics simultaneously to Serial COM1,
 * VGA Text Mode (0xB8000), and the VBE Graphics Framebuffer (if active).
 *
 * @file panic.c
 * @author Bengal Tiger OS (BengalEmpire)
 * @version 1.0.0
 */

#include "panic.h"
#include "common.h"
#include "serial.h"
#include "vbe.h"
#include "font.h"
#include "version.h"

const char *exception_names[32] = {
    "Division By Zero (#DE)",
    "Debug (#DB)",
    "Non-Maskable Interrupt (#NMI)",
    "Breakpoint (#BP)",
    "Into Detected Overflow (#OF)",
    "Bound Range Exceeded (#BR)",
    "Invalid Opcode (#UD)",
    "Device Not Available (#NM)",
    "Double Fault (#DF)",
    "Coprocessor Segment Overrun",
    "Invalid TSS (#TS)",
    "Segment Not Present (#NP)",
    "Stack-Segment Fault (#SS)",
    "General Protection Fault (#GP)",
    "Page Fault (#PF)",
    "Reserved Exception 15",
    "x87 Floating-Point Error (#MF)",
    "Alignment Check (#AC)",
    "Machine Check (#MC)",
    "SIMD Floating-Point (#XM)",
    "Virtualization Exception (#VE)",
    "Control Protection (#CP)",
    "Reserved", "Reserved", "Reserved", "Reserved", "Reserved",
    "Reserved", "Reserved", "Reserved", "Security Exception (#SX)", "Reserved"
};

static volatile uint16_t *panic_vga = (volatile uint16_t *)0xB8000;

#define PANIC_COLOR_HEADER  0x4F
#define PANIC_COLOR_TEXT    0x4E
#define PANIC_COLOR_LABEL   0x47

#define PANIC_VBE_BG        0x00580808
#define PANIC_VBE_BANNER    0x00881111
#define PANIC_VBE_WHITE     0x00FFFFFF
#define PANIC_VBE_YELLOW    0x00FFEE55
#define PANIC_VBE_GREY      0x00DDDDDD

static void format_hex32(uint32_t value, char *hex) {
    const char *hex_chars = "0123456789ABCDEF";
    hex[0] = '0';
    hex[1] = 'x';
    for (int i = 9; i >= 2; i--) {
        hex[i] = hex_chars[value & 0xF];
        value >>= 4;
    }
    hex[10] = '\0';
}

static void panic_draw_str(const char *str, int row, int col, uint8_t vga_attr,
                           uint32_t vbe_fg, uint32_t vbe_bg) {
    int offset = row * 80 + col;
    for (int i = 0; str[i]; i++) {
        if (str[i] != '\n' && offset < 80 * 25) {
            panic_vga[offset++] = (uint16_t)(uint8_t)str[i] | ((uint16_t)vga_attr << 8);
        }
    }

    if (vbe_is_active()) {
        uint32_t x = (uint32_t)(col * FONT_WIDTH + 16);
        uint32_t y = (uint32_t)(row * (FONT_HEIGHT + 2) + 16);
        vbe_draw_string(x, y, str, vbe_fg, vbe_bg);
    }
}

static void panic_draw_hex(uint32_t val, int row, int col) {
    char hex[11];
    format_hex32(val, hex);
    panic_draw_str(hex, row, col, PANIC_COLOR_TEXT, PANIC_VBE_YELLOW, PANIC_VBE_BG);
}

static void panic_begin_screen(const char *title) {
    for (int i = 0; i < 80 * 25; i++) {
        panic_vga[i] = (uint16_t)' ' | ((uint16_t)PANIC_COLOR_TEXT << 8);
    }
    if (vbe_is_active()) {
        vbe_clear_color(PANIC_VBE_BG);
        vbe_fill_rect(0, 0, vbe_fb.width, 68, PANIC_VBE_BANNER);
    }

    panic_draw_str("================================================================================",
                   0, 0, PANIC_COLOR_HEADER, PANIC_VBE_WHITE, PANIC_VBE_BANNER);
    panic_draw_str(title, 1, 14, PANIC_COLOR_HEADER, PANIC_VBE_WHITE, PANIC_VBE_BANNER);
    panic_draw_str("================================================================================",
                   2, 0, PANIC_COLOR_HEADER, PANIC_VBE_WHITE, PANIC_VBE_BANNER);
}

void kernel_panic(const char *message) {
    __asm__ volatile("cli");

    serial_write_str("\n!!! KERNEL PANIC: ");
    serial_write_str(message ? message : "(null)");
    serial_write_str(" !!!\n");

    panic_begin_screen("BENGAL TIGER OS v" OS_VERSION " - KERNEL PANIC");
    panic_draw_str("FATAL ERROR:", 4, 2, PANIC_COLOR_LABEL, PANIC_VBE_WHITE, PANIC_VBE_BG);
    panic_draw_str(message ? message : "Unknown error", 5, 4, PANIC_COLOR_TEXT, PANIC_VBE_YELLOW, PANIC_VBE_BG);
    panic_draw_str("System halted safely. Please reboot the machine.", 16, 2, PANIC_COLOR_LABEL, PANIC_VBE_GREY, PANIC_VBE_BG);

    for (;;) {
        __asm__ volatile("hlt");
    }
}

void kernel_panic_at(const char *message, const char *file, int line) {
    __asm__ volatile("cli");

    char line_str[16];
    k_itoa(line, line_str, 10);

    serial_write_str("\n!!! KERNEL PANIC at ");
    serial_write_str(file ? file : "?");
    serial_write_str(":");
    serial_write_str(line_str);
    serial_write_str(" -> ");
    serial_write_str(message ? message : "?");
    serial_write_str(" !!!\n");

    panic_begin_screen("BENGAL TIGER OS v" OS_VERSION " - KERNEL PANIC");
    panic_draw_str("FATAL ERROR:", 4, 2, PANIC_COLOR_LABEL, PANIC_VBE_WHITE, PANIC_VBE_BG);
    panic_draw_str(message ? message : "Assertion failure", 5, 4, PANIC_COLOR_TEXT, PANIC_VBE_YELLOW, PANIC_VBE_BG);
    panic_draw_str("File:", 7, 2, PANIC_COLOR_LABEL, PANIC_VBE_GREY, PANIC_VBE_BG);
    panic_draw_str(file ? file : "?", 7, 10, PANIC_COLOR_TEXT, PANIC_VBE_YELLOW, PANIC_VBE_BG);
    panic_draw_str("Line:", 8, 2, PANIC_COLOR_LABEL, PANIC_VBE_GREY, PANIC_VBE_BG);
    panic_draw_str(line_str, 8, 10, PANIC_COLOR_TEXT, PANIC_VBE_YELLOW, PANIC_VBE_BG);
    panic_draw_str("System halted safely. Please reboot the machine.", 14, 2, PANIC_COLOR_LABEL, PANIC_VBE_GREY, PANIC_VBE_BG);

    for (;;) {
        __asm__ volatile("hlt");
    }
}

void exception_handler(struct regs *r) {
    if (r->int_no < 32) {
        __asm__ volatile("cli");

        serial_write_str("\n!!! CPU EXCEPTION: ");
        serial_write_str(exception_names[r->int_no]);
        serial_write_str(" EIP=");
        serial_write_hex(COM1_PORT, r->eip);
        serial_write_str(" ERR=");
        serial_write_hex(COM1_PORT, r->err_code);
        serial_write_str(" !!!\n");

        panic_begin_screen("BENGAL TIGER OS v" OS_VERSION " - CPU EXCEPTION");

        panic_draw_str("EXCEPTION:", 4, 2, PANIC_COLOR_LABEL, PANIC_VBE_WHITE, PANIC_VBE_BG);
        panic_draw_str(exception_names[r->int_no], 4, 14, PANIC_COLOR_TEXT, PANIC_VBE_YELLOW, PANIC_VBE_BG);

        panic_draw_str("REGISTER DUMP:", 6, 2, PANIC_COLOR_LABEL, PANIC_VBE_WHITE, PANIC_VBE_BG);

        panic_draw_str("EAX:", 7, 4, PANIC_COLOR_LABEL, PANIC_VBE_GREY, PANIC_VBE_BG);  panic_draw_hex(r->eax, 7, 9);
        panic_draw_str("EBX:", 7, 22, PANIC_COLOR_LABEL, PANIC_VBE_GREY, PANIC_VBE_BG); panic_draw_hex(r->ebx, 7, 27);
        panic_draw_str("ECX:", 7, 40, PANIC_COLOR_LABEL, PANIC_VBE_GREY, PANIC_VBE_BG); panic_draw_hex(r->ecx, 7, 45);
        panic_draw_str("EDX:", 7, 58, PANIC_COLOR_LABEL, PANIC_VBE_GREY, PANIC_VBE_BG); panic_draw_hex(r->edx, 7, 63);

        panic_draw_str("ESI:", 8, 4, PANIC_COLOR_LABEL, PANIC_VBE_GREY, PANIC_VBE_BG);  panic_draw_hex(r->esi, 8, 9);
        panic_draw_str("EDI:", 8, 22, PANIC_COLOR_LABEL, PANIC_VBE_GREY, PANIC_VBE_BG); panic_draw_hex(r->edi, 8, 27);
        panic_draw_str("EBP:", 8, 40, PANIC_COLOR_LABEL, PANIC_VBE_GREY, PANIC_VBE_BG); panic_draw_hex(r->ebp, 8, 45);
        panic_draw_str("ESP:", 8, 58, PANIC_COLOR_LABEL, PANIC_VBE_GREY, PANIC_VBE_BG); panic_draw_hex(r->esp, 8, 63);

        panic_draw_str("EIP:", 9, 4, PANIC_COLOR_LABEL, PANIC_VBE_GREY, PANIC_VBE_BG);  panic_draw_hex(r->eip, 9, 9);
        panic_draw_str("CS:", 9, 22, PANIC_COLOR_LABEL, PANIC_VBE_GREY, PANIC_VBE_BG);  panic_draw_hex(r->cs, 9, 27);
        panic_draw_str("EFLAGS:", 9, 40, PANIC_COLOR_LABEL, PANIC_VBE_GREY, PANIC_VBE_BG); panic_draw_hex(r->eflags, 9, 48);

        if (r->int_no == 14) {
            uint32_t cr2;
            __asm__ volatile("mov %%cr2, %0" : "=r"(cr2));
            serial_write_str("CR2=");
            serial_write_hex(COM1_PORT, cr2);
            serial_write_str("\n");

            panic_draw_str("PAGE FAULT ADDRESS (CR2):", 11, 2, PANIC_COLOR_LABEL, PANIC_VBE_WHITE, PANIC_VBE_BG);
            panic_draw_hex(cr2, 11, 28);
            panic_draw_str("Error code:", 12, 2, PANIC_COLOR_LABEL, PANIC_VBE_GREY, PANIC_VBE_BG);
            panic_draw_hex(r->err_code, 12, 15);
        }

        panic_draw_str("The system has been halted safely. Please restart your computer.",
                       15, 2, PANIC_COLOR_LABEL, PANIC_VBE_GREY, PANIC_VBE_BG);

        for (;;) {
            __asm__ volatile("hlt");
        }
    }
}

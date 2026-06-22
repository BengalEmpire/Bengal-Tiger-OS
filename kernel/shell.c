/**
 * Bengal Tiger OS - Enhanced Shell Implementation
 * 
 * Full-featured command-line interface.
 * 
 * @file shell.c
 * @author Bengal Tiger OS (BengalEmpire)
 * @version 0.3.0
 */

#include "shell.h"
#include "common.h"
#include "fat.h"
#include "timer.h"
#include "heap.h"
#include "pci.h"
#include "paging.h"
#include "rtc.h"
#include "cpu.h"
#include "serial.h"
#include "disk.h"
#include "vbe.h"
#include "mouse.h"

/* Shell state */
char shell_cmd_buf[SHELL_CMD_MAX_LENGTH];
int shell_cmd_pos = 0;
char shell_username[32] = "user";
int shell_setup_mode = 0;
volatile uint16_t *video = (volatile uint16_t*)0xB8000;
int shell_offset = 0;

/* Command history */
static char cmd_history[SHELL_HISTORY_SIZE][SHELL_CMD_MAX_LENGTH];
static int history_count = 0;
static int history_index = -1;

/* Cursor position within command buffer */
static int cursor_pos = 0;

/* Default color (Yellow on Black - Bengal Tiger theme) */
static uint8_t default_color = VGA_COLOR_YELLOW | (VGA_COLOR_BLACK << 4);

/* Tiger ASCII art for boot */
static const char *tiger_art[] = {
    "                                  _.._",
    "                                .'    `.",
    "                               /   __   \\",
    "                              |   /  \\  |",
    "                              |   \\__/  |",
    "                              `.   ||   .'",
    "                                `-.||.-'",
    "                               .-'    `-.",
    "                              /  BENGAL  \\",
    "                             |   TIGER   |",
    "                             |    OS     |",
    "                              \\  v0.6.0  /",
    "                               `-.____..-'",
    NULL
};

/* Forward declarations */
static void show_prompt(void);
static void scroll(void);

/* Helper: Make VGA color attribute */
static inline uint8_t make_color(uint8_t fg, uint8_t bg) {
    return fg | (bg << 4);
}

static void scroll(void) {
    if (shell_offset >= 80 * 25) {
        /* Move lines up */
        for (int i = 0; i < 24 * 80; i++) {
            video[i] = video[i + 80];
        }
        /* Clear last line */
        for (int i = 24 * 80; i < 25 * 80; i++) {
            video[i] = ' ' | (default_color << 8);
        }
        shell_offset = 24 * 80;
    }
}

void shell_print_color(const char *s, uint8_t color) {
    int len = strlen(s);
    for (int i = 0; i < len; ++i) {
        if (s[i] == '\n') {
            shell_offset = (shell_offset / 80 + 1) * 80;
        } else if (s[i] == '\t') {
            /* Tab - align to 8 spaces */
            do {
                video[shell_offset] = ' ' | ((uint16_t)color << 8);
                shell_offset++;
            } while (shell_offset % 8 != 0);
        } else {
            video[shell_offset] = (uint16_t)s[i] | ((uint16_t)color << 8);
            shell_offset++;
        }
        scroll();
    }
}

void shell_print(const char *s) {
    shell_print_color(s, default_color);
}

void shell_print_int(int num) {
    char buf[16];
    int pos = 0;
    
    if (num < 0) {
        shell_print("-");
        num = -num;
    }
    
    if (num == 0) {
        shell_print("0");
        return;
    }
    
    while (num > 0) {
        buf[pos++] = '0' + (num % 10);
        num /= 10;
    }
    
    /* Reverse */
    while (pos > 0) {
        char c[2] = {buf[--pos], 0};
        shell_print(c);
    }
}

void shell_print_hex(uint32_t num) {
    const char *hex = "0123456789ABCDEF";
    char buf[11] = "0x00000000";
    
    for (int i = 9; i >= 2; i--) {
        buf[i] = hex[num & 0xF];
        num >>= 4;
    }
    
    shell_print(buf);
}

void shell_clear_screen(void) {
    for (int i = 0; i < 80 * 25; i++) {
        video[i] = ' ' | (default_color << 8);
    }
    shell_offset = 0;
}

static void show_prompt(void) {
    shell_print_color(shell_username, make_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK));
    shell_print_color("@", make_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK));
    shell_print_color("bengal-tiger", make_color(VGA_COLOR_LIGHT_CYAN, VGA_COLOR_BLACK));
    shell_print_color(":~$ ", make_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK));
}

static void add_to_history(const char *cmd) {
    if (strlen(cmd) == 0) return;
    
    /* Don't add duplicates */
    if (history_count > 0 && strcmp(cmd_history[(history_count - 1) % SHELL_HISTORY_SIZE], cmd) == 0) {
        return;
    }
    
    /* Add to history */
    memcpy(cmd_history[history_count % SHELL_HISTORY_SIZE], cmd, SHELL_CMD_MAX_LENGTH);
    history_count++;
    history_index = -1;
}

/* Redraw the current command line */
static void redraw_command_line(void) {
    /* Go back to start of command */
    int prompt_start = shell_offset - shell_cmd_pos;
    shell_offset = prompt_start;
    
    /* Clear rest of line */
    int clear_len = SHELL_CMD_MAX_LENGTH < (80 - (shell_offset % 80)) ? 80 - (shell_offset % 80) : SHELL_CMD_MAX_LENGTH;
    for (int i = 0; i < clear_len; i++) {
        video[shell_offset + i] = ' ' | (default_color << 8);
    }
    
    /* Print command */
    shell_print(shell_cmd_buf);
    
    /* Move cursor to correct position */
    shell_offset = prompt_start + cursor_pos;
}

/* ==================== COMMAND IMPLEMENTATIONS ==================== */

static void cmd_help(void) {
    shell_print_color("\n=== Bengal Tiger OS Commands ===\n", make_color(VGA_COLOR_LIGHT_CYAN, VGA_COLOR_BLACK));
    shell_print_color("\nFile Commands:\n", make_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK));
    shell_print("  ls            - List files\n");
    shell_print("  cat <file>    - Display file contents\n");
    
    shell_print_color("\nSystem Commands:\n", make_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK));
    shell_print("  info          - Show OS version\n");
    shell_print("  neofetch      - Display system info with style\n");
    shell_print("  uptime        - Show system uptime\n");
    shell_print("  mem           - Show memory usage\n");
    shell_print("  pci           - List PCI devices\n");
    shell_print("  date          - Show current date/time\n");
    
    shell_print_color("\nShell Commands:\n", make_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK));
    shell_print("  echo <msg>    - Print message\n");
    shell_print("  clear         - Clear screen\n");
    shell_print("  color <n>     - Set text color (0-15)\n");
    shell_print("  history       - Show command history\n");
    shell_print("  help          - Show this help\n");
    
    shell_print_color("\nControl Commands:\n", make_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK));
    shell_print("  reboot        - Reboot the system\n");
    shell_print("  shutdown      - Halt the system\n");
    shell_print("  exit          - Alias for shutdown\n");

    shell_print_color("\nHardware Commands:\n", make_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK));
    shell_print("  disk          - Show disk info\n");
    shell_print("  cpu           - Show CPU information\n");
    shell_print("  vbe           - Show VBE framebuffer info & demo\n");
    shell_print("  mouse         - Show mouse status & test cursor\n");

    shell_print_color("\nTips:\n", make_color(VGA_COLOR_YELLOW, VGA_COLOR_BLACK));
    shell_print("  * Use UP/DOWN arrows for command history\n");
    shell_print("  * Shift+Letter for uppercase\n\n");
}

static void cmd_neofetch(void) {
    shell_print("\n");
    
    /* ASCII Art + System Info side by side */
    shell_print_color("        ,'/", make_color(VGA_COLOR_LIGHT_RED, VGA_COLOR_BLACK));
    shell_print_color("                 ", make_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK));
    shell_print_color(shell_username, make_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK));
    shell_print_color("@", make_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK));
    shell_print_color("bengal-tiger\n", make_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK));
    
    shell_print_color("       / /", make_color(VGA_COLOR_LIGHT_RED, VGA_COLOR_BLACK));
    shell_print("                  ----------------\n");
    
    shell_print_color("      / /", make_color(VGA_COLOR_LIGHT_RED, VGA_COLOR_BLACK));
    shell_print_color("                   OS", make_color(VGA_COLOR_LIGHT_CYAN, VGA_COLOR_BLACK));
    shell_print(": Bengal Tiger OS\n");
    
    shell_print_color("   .-' '-.", make_color(VGA_COLOR_YELLOW, VGA_COLOR_BLACK));
    shell_print_color("                 Version", make_color(VGA_COLOR_LIGHT_CYAN, VGA_COLOR_BLACK));
    shell_print(": 0.3.0\n");
    
    shell_print_color("  /  ___  \\", make_color(VGA_COLOR_YELLOW, VGA_COLOR_BLACK));
    shell_print_color("                Kernel", make_color(VGA_COLOR_LIGHT_CYAN, VGA_COLOR_BLACK));
    shell_print(": i386 32-bit\n");
    
    shell_print_color("  | |   | |", make_color(VGA_COLOR_YELLOW, VGA_COLOR_BLACK));
    shell_print_color("                Uptime", make_color(VGA_COLOR_LIGHT_CYAN, VGA_COLOR_BLACK));
    shell_print(": ");
    char uptime_buf[32];
    timer_format_uptime(uptime_buf);
    shell_print(uptime_buf);
    shell_print("\n");
    
    shell_print_color("  | |   | |", make_color(VGA_COLOR_YELLOW, VGA_COLOR_BLACK));
    shell_print_color("                Shell", make_color(VGA_COLOR_LIGHT_CYAN, VGA_COLOR_BLACK));
    shell_print(": btsh (Bengal Tiger Shell)\n");
    
    shell_print_color("  | |===| |", make_color(VGA_COLOR_YELLOW, VGA_COLOR_BLACK));
    shell_print_color("                Terminal", make_color(VGA_COLOR_LIGHT_CYAN, VGA_COLOR_BLACK));
    shell_print(": 80x25 VGA\n");
    
    shell_print_color("  |_|   |_|", make_color(VGA_COLOR_YELLOW, VGA_COLOR_BLACK));
    shell_print_color("                CPU", make_color(VGA_COLOR_LIGHT_CYAN, VGA_COLOR_BLACK));
    shell_print(": i386 compatible\n");
    
    shell_print_color("   \\_   _/", make_color(VGA_COLOR_BROWN, VGA_COLOR_BLACK));
    shell_print_color("                 Memory", make_color(VGA_COLOR_LIGHT_CYAN, VGA_COLOR_BLACK));
    shell_print(": ");
    heap_stats_t stats;
    heap_get_stats(&stats);
    shell_print_int(stats.used_size / 1024);
    shell_print("K / ");
    shell_print_int(stats.total_size / 1024);
    shell_print("K\n");
    
    shell_print_color("    |_____|", make_color(VGA_COLOR_BROWN, VGA_COLOR_BLACK));
    shell_print_color("                PCI Devices", make_color(VGA_COLOR_LIGHT_CYAN, VGA_COLOR_BLACK));
    shell_print(": ");
    shell_print_int(pci_get_device_count());
    shell_print("\n");
    
    /* If VBE is active, also show graphics mode info in neofetch */
    if (vbe_is_active()) {
        shell_print_color("    |_____|", make_color(VGA_COLOR_BROWN, VGA_COLOR_BLACK));
        shell_print_color("                Graphics", make_color(VGA_COLOR_LIGHT_CYAN, VGA_COLOR_BLACK));
        shell_print(": ");
        shell_print_int(vbe_fb.width);
        shell_print("x");
        shell_print_int(vbe_fb.height);
        shell_print("x");
        shell_print_int(vbe_fb.bpp);
        shell_print(" VBE\n");
    }

    /* Color blocks */
    shell_print("\n                            ");
    for (int i = 0; i < 8; i++) {
        uint8_t color = make_color(VGA_COLOR_BLACK, i);
        video[shell_offset++] = ' ' | (color << 8);
        video[shell_offset++] = ' ' | (color << 8);
    }
    shell_print("\n                            ");
    for (int i = 8; i < 16; i++) {
        uint8_t color = make_color(VGA_COLOR_BLACK, i);
        video[shell_offset++] = ' ' | (color << 8);
        video[shell_offset++] = ' ' | (color << 8);
    }
    shell_print("\n\n");
}

static void cmd_uptime(void) {
    char buf[32];
    timer_format_uptime(buf);
    shell_print("Uptime: ");
    shell_print(buf);
    shell_print("\n");
}

static void cmd_mem(void) {
    heap_stats_t stats;
    heap_get_stats(&stats);
    
    shell_print_color("\n=== Memory Usage ===\n", make_color(VGA_COLOR_LIGHT_CYAN, VGA_COLOR_BLACK));
    
    shell_print("Heap Total:       ");
    shell_print_int(stats.total_size / 1024);
    shell_print(" KB\n");
    
    shell_print("Heap Used:        ");
    shell_print_int(stats.used_size / 1024);
    shell_print(" KB\n");
    
    shell_print("Heap Free:        ");
    shell_print_int(stats.free_size / 1024);
    shell_print(" KB\n");
    
    shell_print("Allocations:      ");
    shell_print_int(stats.num_allocations);
    shell_print("\n");
    
    shell_print("Heap Blocks:      ");
    shell_print_int(stats.num_blocks);
    shell_print("\n");
    
    shell_print("Heap Integrity:   ");
    if (heap_check()) {
        shell_print_color("OK\n", make_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK));
    } else {
        shell_print_color("CORRUPTED!\n", make_color(VGA_COLOR_LIGHT_RED, VGA_COLOR_BLACK));
    }
    shell_print("\n");
}

static void cmd_pci(void) {
    uint32_t count = pci_get_device_count();
    
    shell_print_color("\n=== PCI Devices (", make_color(VGA_COLOR_LIGHT_CYAN, VGA_COLOR_BLACK));
    shell_print_int(count);
    shell_print_color(" found) ===\n\n", make_color(VGA_COLOR_LIGHT_CYAN, VGA_COLOR_BLACK));
    
    if (count == 0) {
        shell_print("No PCI devices detected.\n");
        return;
    }
    
    shell_print_color("Bus Dev Fn  Vendor   Device   Class\n", make_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK));
    shell_print("--- --- --  ------   ------   -----\n");
    
    for (uint32_t i = 0; i < count && i < 12; i++) {  /* Limit to 12 to fit screen */
        pci_device_t *dev = pci_get_device(i);
        if (!dev) continue;
        
        /* Bus */
        if (dev->bus < 10) shell_print(" ");
        shell_print_int(dev->bus);
        shell_print("  ");
        
        /* Device */
        if (dev->device < 10) shell_print(" ");
        shell_print_int(dev->device);
        shell_print("  ");
        
        /* Function */
        shell_print_int(dev->function);
        shell_print("   ");
        
        /* Vendor ID */
        shell_print_hex(dev->vendor_id);
        shell_print(" ");
        
        /* Device ID */
        shell_print_hex(dev->device_id);
        shell_print(" ");
        
        /* Class name */
        shell_print_color(pci_class_name(dev->class_code), make_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK));
        shell_print("\n");
    }
    
    if (count > 12) {
        shell_print("... and ");
        shell_print_int(count - 12);
        shell_print(" more devices\n");
    }
    shell_print("\n");
}

static void cmd_show_history(void) {
    shell_print_color("\n=== Command History ===\n", make_color(VGA_COLOR_LIGHT_CYAN, VGA_COLOR_BLACK));
    
    int start = (history_count > SHELL_HISTORY_SIZE) ? history_count - SHELL_HISTORY_SIZE : 0;
    
    for (int i = start; i < history_count; i++) {
        shell_print_int(i + 1);
        shell_print("  ");
        shell_print(cmd_history[i % SHELL_HISTORY_SIZE]);
        shell_print("\n");
    }
    shell_print("\n");
}

static void cmd_color(const char *arg) {
    int color = 0;
    while (*arg >= '0' && *arg <= '9') {
        color = color * 10 + (*arg - '0');
        arg++;
    }
    
    if (color >= 0 && color < 16) {
        default_color = color | (VGA_COLOR_BLACK << 4);
        shell_print("Color changed.\n");
    } else {
        shell_print("Invalid color. Use 0-15.\n");
    }
}

static void cmd_reboot(void) {
    shell_print_color("\nRebooting...\n", make_color(VGA_COLOR_LIGHT_RED, VGA_COLOR_BLACK));

    /* Method 1: Try keyboard controller reset (works on most real HW) */
    int timeout = 10000;
    while (timeout-- && (inb(0x64) & 0x02));
    if (timeout > 0) {
        outb(0x64, 0xFE);
    }

    /* Method 2: Try ACPI reset */
    outw(0x604, 0x2000);

    /* Method 3: Triple fault (will cause CPU to reset) */
    __asm__ volatile("cli\n"
        "mov $0x1234, %%eax\n"
        "lidt (%%eax)\n"
        "int $0x03"
        : : : "eax", "memory");

    /* Last resort */
    __asm__ volatile("cli; hlt");
}

static void cmd_shutdown(void) {
    shell_print_color("\nShutting down Bengal Tiger OS...\n", make_color(VGA_COLOR_LIGHT_RED, VGA_COLOR_BLACK));
    shell_print("Goodbye!\n");

    /* Method 1: QEMU/VirtualBox ACPI power off */
    outw(0x604, 0x2000);
    outw(0x604, 0x2000);

    /* Method 2: Bochs/QEMU older method */
    outw(0xB004, 0x2000);

    /* Method 3: VirtualBox ACPI */
    outw(0x4004, 0x3400);

    /* Halt */
    __asm__ volatile("cli; hlt");
}

/* ==================== COMMAND EXECUTION ==================== */

void shell_execute_command(const char *cmd) {
    /* Skip leading whitespace */
    while (*cmd == ' ') cmd++;
    
    if (strlen(cmd) == 0) return;
    
    /* Parse command and first argument */
    char command[64] = {0};
    char arg[192] = {0};
    
    int i = 0;
    while (cmd[i] && cmd[i] != ' ' && i < 63) {
        command[i] = cmd[i];
        i++;
    }
    command[i] = 0;
    
    /* Skip spaces before argument */
    while (cmd[i] == ' ') i++;
    
    /* Copy rest as argument */
    int j = 0;
    while (cmd[i] && j < 191) {
        arg[j++] = cmd[i++];
    }
    arg[j] = 0;
    
    /* Execute command */
    if (strcmp(command, "help") == 0) {
        cmd_help();
    } else if (strcmp(command, "ls") == 0) {
        fat_list_files();
    } else if (strcmp(command, "cat") == 0) {
        if (strlen(arg) == 0) {
            shell_print("Usage: cat <filename>\n");
        } else {
            char file_buf[512] = {0};
            fat_load_file(arg, file_buf);
            if (file_buf[0]) {
                shell_print(file_buf);
                shell_print("\n");
            } else {
                shell_print("File not found: ");
                shell_print(arg);
                shell_print("\n");
            }
        }
    } else if (strcmp(command, "echo") == 0) {
        shell_print(arg);
        shell_print("\n");
    } else if (strcmp(command, "clear") == 0) {
        shell_clear_screen();
    } else if (strcmp(command, "info") == 0) {
        shell_print_color("Bengal Tiger OS ", make_color(VGA_COLOR_YELLOW, VGA_COLOR_BLACK));
        shell_print_color("v0.3.0\n", make_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK));
        shell_print("Built for stability and performance.\n");
        shell_print("Architecture: i386 (32-bit)\n");
    } else if (strcmp(command, "neofetch") == 0) {
        cmd_neofetch();
    } else if (strcmp(command, "uptime") == 0) {
        cmd_uptime();
    } else if (strcmp(command, "mem") == 0) {
        cmd_mem();
    } else if (strcmp(command, "pci") == 0) {
        cmd_pci();
    } else if (strcmp(command, "history") == 0) {
        cmd_show_history();
    } else if (strcmp(command, "color") == 0) {
        cmd_color(arg);
    } else if (strcmp(command, "date") == 0) {
        rtc_time_t time;
        if (rtc_read_time(&time)) {
            char buf[32];
            rtc_format_datetime(&time, buf);
            shell_print_color(buf, make_color(VGA_COLOR_LIGHT_CYAN, VGA_COLOR_BLACK));
            shell_print("\n");
        } else {
            shell_print("RTC not available - showing uptime\n");
            cmd_uptime();
        }
    } else if (strcmp(command, "reboot") == 0) {
        cmd_reboot();
    } else if (strcmp(command, "shutdown") == 0 || strcmp(command, "exit") == 0) {
        cmd_shutdown();
    } else if (strcmp(command, "disk") == 0) {
        ata_drive_t *drive = ata_get_drive_info();
        if (drive && drive->present) {
            shell_print_color("\n=== Disk Information ===\n", make_color(VGA_COLOR_LIGHT_CYAN, VGA_COLOR_BLACK));
            shell_print_color("Model:    ", make_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK));
            shell_print(drive->model);
            shell_print("\n");
            shell_print_color("Serial:   ", make_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK));
            shell_print(drive->serial);
            shell_print("\n");
            shell_print_color("Firmware: ", make_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK));
            shell_print(drive->firmware);
            shell_print("\n");
            shell_print_color("Type:     ", make_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK));
            shell_print(drive->drive_type == ATA_TYPE_ATA ? "ATA Hard Disk" : "ATAPI CD/DVD");
            shell_print("\n");
            shell_print_color("Capacity: ", make_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK));
            uint64_t cap = ata_get_capacity();
            uint32_t mb = (uint32_t)(cap / (1024 * 1024));
            shell_print_int(mb);
            shell_print(" MB");
            if (drive->is_lba48) {
                shell_print(" (LBA48)");
            }
            shell_print("\n\n");
        } else {
            shell_print("\nNo ATA disk detected.\n\n");
        }
    } else if (strcmp(command, "mouse") == 0) {
        if (mouse_is_present()) {
            shell_print_color("\n=== PS/2 Mouse ===\n", make_color(VGA_COLOR_LIGHT_CYAN, VGA_COLOR_BLACK));
            shell_print_color("Status: ", make_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK));
            shell_print("Present\n");
            shell_print_color("Type:   ", make_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK));
            shell_print(mouse_state.has_wheel ? "Wheel Mouse (4-byte)" : "Standard Mouse (3-byte)");
            shell_print("\n");
            shell_print_color("X:      ", make_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK));
            shell_print_int(mouse_get_x());
            shell_print("\n");
            shell_print_color("Y:      ", make_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK));
            shell_print_int(mouse_get_y());
            shell_print("\n");
            shell_print_color("Btns:   ", make_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK));
            uint8_t btns = mouse_get_buttons();
            shell_print(btns & MOUSE_BTN_LEFT ? "L" : "-");
            shell_print(btns & MOUSE_BTN_MIDDLE ? "M" : "-");
            shell_print(btns & MOUSE_BTN_RIGHT ? "R" : "-");
            shell_print("\n");
            if (vbe_is_active()) {
                shell_print("\nCursor visible on framebuffer. Move the mouse!\n");
            } else {
                shell_print("\nSwitch to graphics mode (VBE) to see the cursor.\n");
            }
            shell_print("\n");
        } else {
            shell_print("\nNo PS/2 mouse detected.\n");
            shell_print("Ensure PS/2 mouse is connected and IRQ12 is unmasked.\n\n");
        }
    } else if (strcmp(command, "vbe") == 0) {
        if (vbe_is_active()) {
            shell_print_color("\n=== VBE Framebuffer ===\n", make_color(VGA_COLOR_LIGHT_CYAN, VGA_COLOR_BLACK));
            shell_print_color("Mode:     ", make_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK));
            shell_print_int(vbe_fb.width);
            shell_print("x");
            shell_print_int(vbe_fb.height);
            shell_print("x");
            shell_print_int(vbe_fb.bpp);
            shell_print("\n");
            shell_print_color("Pitch:    ", make_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK));
            shell_print_int(vbe_fb.pitch);
            shell_print(" bytes\n");
            shell_print_color("FB Addr:  ", make_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK));
            shell_print_hex(vbe_fb.phys_addr);
            shell_print("\n");
            shell_print_color("Red:      ", make_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK));
            shell_print_int(vbe_fb.red_size);
            shell_print(" bits at pos ");
            shell_print_int(vbe_fb.red_pos);
            shell_print("\n");
            shell_print_color("Green:    ", make_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK));
            shell_print_int(vbe_fb.green_size);
            shell_print(" bits at pos ");
            shell_print_int(vbe_fb.green_pos);
            shell_print("\n");
            shell_print_color("Blue:     ", make_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK));
            shell_print_int(vbe_fb.blue_size);
            shell_print(" bits at pos ");
            shell_print_int(vbe_fb.blue_pos);
            shell_print("\n");
            shell_print_color("FB Size:  ", make_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK));
            uint32_t fb_mb = (vbe_fb.pitch * vbe_fb.height) / (1024 * 1024);
            if (fb_mb > 0) {
                shell_print_int(fb_mb);
                shell_print(" MB\n");
            } else {
                shell_print_int((vbe_fb.pitch * vbe_fb.height) / 1024);
                shell_print(" KB\n");
            }

            /* Draw a demo pattern on the framebuffer */
            shell_print_color("\nDrawing demo pattern...\n", make_color(VGA_COLOR_YELLOW, VGA_COLOR_BLACK));

            /* Color bars across top */
            uint32_t bar_w = vbe_fb.width / 16;
            uint32_t bar_h = 48;
            for (int i = 0; i < 16 && i * bar_w < vbe_fb.width; i++) {
                vbe_fill_rect(i * bar_w, 0, bar_w, bar_h, vbe_palette[i]);
            }

            /* Draw a cyan border rectangle */
            vbe_draw_rect(0, 0, vbe_fb.width - 1, vbe_fb.height - 1, VBE_CYAN, 3);

            /* Draw some text in the framebuffer */
            uint32_t tx = 20;
            uint32_t ty = bar_h + 12;
            vbe_draw_string(tx, ty, "Bengal Tiger OS - Graphics Mode Active!", VBE_YELLOW, VBE_BLACK);
            ty += 20;
            vbe_draw_string(tx, ty, "Resolution: 1024x768 @ 32bpp", VBE_LIGHT_CYAN, VBE_BLACK);
            ty += 20;
            vbe_draw_string(tx, ty, "Try: vbe, vbe demo, vbe clear", VBE_LIGHT_GREEN, VBE_BLACK);

            shell_print("Done! The framebuffer now has color bars and text.\n\n");
        } else {
            shell_print("\nVBE framebuffer is not active.\n");
            shell_print("Boot using a graphics mode entry in GRUB (e.g., '1024x768x32').\n\n");
        }
    } else if (strcmp(command, "cpu") == 0) {
        shell_print_color("\n=== CPU Information ===\n", make_color(VGA_COLOR_LIGHT_CYAN, VGA_COLOR_BLACK));
        shell_print_color("Vendor:  ", make_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK));
        shell_print(cpu_get_vendor());
        shell_print("\n");
        shell_print_color("Brand:   ", make_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK));
        shell_print(cpu_get_brand());
        shell_print("\n");
        if (cpu_info.has_fpu) {
            shell_print_color("FPU:     ", make_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK));
            shell_print("Present\n");
        }
        if (cpu_info.has_sse || cpu_info.has_sse2) {
            shell_print_color("SSE:    ", make_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK));
            shell_print(cpu_info.has_sse2 ? "SSE2" : "SSE");
            shell_print("\n");
        }
        shell_print("\n");
    } else {
        shell_print_color("Unknown command: ", make_color(VGA_COLOR_LIGHT_RED, VGA_COLOR_BLACK));
        shell_print(command);
        shell_print("\nType 'help' for available commands.\n");
    }
}

/* ==================== INPUT HANDLING ==================== */

void shell_handler(char ch) {
    if (shell_setup_mode) {
        /* First-time username setup */
        if (ch == '\n') {
            shell_cmd_buf[shell_cmd_pos] = 0;
            memcpy(shell_username, shell_cmd_buf, 31);
            shell_setup_mode = 0;
            shell_clear_screen();
            shell_init(shell_username);
        } else {
            if (shell_cmd_pos < SHELL_CMD_MAX_LENGTH - 1) {
                shell_cmd_buf[shell_cmd_pos++] = ch;
                char temp[2] = {ch, 0};
                shell_print(temp);
            }
        }
        return;
    }

    if (ch == '\n') {
        shell_cmd_buf[shell_cmd_pos] = 0;
        shell_print("\n");
        
        /* Add to history and execute */
        add_to_history(shell_cmd_buf);
        shell_execute_command(shell_cmd_buf);
        
        /* Reset */
        shell_cmd_pos = 0;
        cursor_pos = 0;
        memset(shell_cmd_buf, 0, SHELL_CMD_MAX_LENGTH);
        show_prompt();
        
    } else if (ch == '\b') {
        /* Backspace */
        if (cursor_pos > 0) {
            /* Shift characters left */
            for (int i = cursor_pos - 1; i < shell_cmd_pos - 1; i++) {
                shell_cmd_buf[i] = shell_cmd_buf[i + 1];
            }
            shell_cmd_buf[shell_cmd_pos - 1] = 0;
            shell_cmd_pos--;
            cursor_pos--;
            
            /* Handle screen update */
            shell_offset--;
            
            /* Redraw */
            redraw_command_line();
        }
    } else if (ch == '\t') {
        /* Tab - could implement completion here */
        /* For now, just insert spaces */
        int spaces = 4 - (cursor_pos % 4);
        for (int i = 0; i < spaces && shell_cmd_pos < SHELL_CMD_MAX_LENGTH - 1; i++) {
            shell_cmd_buf[shell_cmd_pos++] = ' ';
            cursor_pos++;
            shell_print(" ");
        }
    } else {
        /* Regular character */
        if (shell_cmd_pos < SHELL_CMD_MAX_LENGTH - 1) {
            /* Insert at cursor position */
            if (cursor_pos < shell_cmd_pos) {
                /* Shift characters right */
                for (int i = shell_cmd_pos; i > cursor_pos; i--) {
                    shell_cmd_buf[i] = shell_cmd_buf[i - 1];
                }
            }
            shell_cmd_buf[cursor_pos] = ch;
            shell_cmd_pos++;
            cursor_pos++;
            
            /* Display character */
            char temp[2] = {ch, 0};
            shell_print(temp);
        }
    }
}

void shell_handler_special(int key) {
    if (shell_setup_mode) return;
    
    switch (key) {
        case SPECIAL_KEY_UP:
            /* Previous command in history */
            if (history_count > 0) {
                if (history_index == -1) {
                    history_index = history_count - 1;
                } else if (history_index > 0) {
                    history_index--;
                }
                
                /* Load command from history */
                memcpy(shell_cmd_buf, cmd_history[history_index % SHELL_HISTORY_SIZE], SHELL_CMD_MAX_LENGTH);
                shell_cmd_pos = strlen(shell_cmd_buf);
                cursor_pos = shell_cmd_pos;
                redraw_command_line();
            }
            break;
            
        case SPECIAL_KEY_DOWN:
            /* Next command in history */
            if (history_index >= 0) {
                history_index++;
                if (history_index >= history_count) {
                    /* Clear command line */
                    history_index = -1;
                    memset(shell_cmd_buf, 0, SHELL_CMD_MAX_LENGTH);
                    shell_cmd_pos = 0;
                    cursor_pos = 0;
                } else {
                    memcpy(shell_cmd_buf, cmd_history[history_index % SHELL_HISTORY_SIZE], SHELL_CMD_MAX_LENGTH);
                    shell_cmd_pos = strlen(shell_cmd_buf);
                    cursor_pos = shell_cmd_pos;
                }
                redraw_command_line();
            }
            break;
            
        case SPECIAL_KEY_LEFT:
            if (cursor_pos > 0) {
                cursor_pos--;
                shell_offset--;
            }
            break;
            
        case SPECIAL_KEY_RIGHT:
            if (cursor_pos < shell_cmd_pos) {
                cursor_pos++;
                shell_offset++;
            }
            break;
            
        case SPECIAL_KEY_HOME:
            shell_offset -= cursor_pos;
            cursor_pos = 0;
            break;
            
        case SPECIAL_KEY_END:
            shell_offset += (shell_cmd_pos - cursor_pos);
            cursor_pos = shell_cmd_pos;
            break;
            
        case SPECIAL_KEY_DELETE:
            if (cursor_pos < shell_cmd_pos) {
                /* Shift characters left */
                for (int i = cursor_pos; i < shell_cmd_pos - 1; i++) {
                    shell_cmd_buf[i] = shell_cmd_buf[i + 1];
                }
                shell_cmd_buf[shell_cmd_pos - 1] = 0;
                shell_cmd_pos--;
                redraw_command_line();
            }
            break;
    }
}

void shell_boot_animation(void) {
    shell_clear_screen();
    
    /* Display tiger art with animation */
    sleep_ms(200);
    
    for (int i = 0; tiger_art[i] != NULL; i++) {
        shell_print_color(tiger_art[i], make_color(VGA_COLOR_YELLOW, VGA_COLOR_BLACK));
        shell_print("\n");
        sleep_ms(100);
    }
    
    sleep_ms(500);
    
    /* Boot progress */
    shell_print("\n");
    shell_print_color("  Initializing hardware", make_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK));
    for (int i = 0; i < 3; i++) {
        sleep_ms(300);
        shell_print(".");
    }
    shell_print_color(" OK\n", make_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK));
    
    shell_print_color("  Loading kernel modules", make_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK));
    for (int i = 0; i < 3; i++) {
        sleep_ms(300);
        shell_print(".");
    }
    shell_print_color(" OK\n", make_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK));
    
    shell_print_color("  Starting shell", make_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK));
    for (int i = 0; i < 3; i++) {
        sleep_ms(300);
        shell_print(".");
    }
    shell_print_color(" OK\n", make_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK));
    
    sleep_ms(500);
}

void shell_init(const char *username) {
    memcpy(shell_username, username, 31);
    shell_username[31] = 0;
    
    shell_cmd_pos = 0;
    cursor_pos = 0;
    memset(shell_cmd_buf, 0, SHELL_CMD_MAX_LENGTH);
    
    shell_clear_screen();
    
    /* Welcome message */
    shell_print_color("Bengal Tiger OS ", make_color(VGA_COLOR_YELLOW, VGA_COLOR_BLACK));
    shell_print_color("v0.3.0", make_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK));
    shell_print_color(" - Type 'help' for commands\n\n", make_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK));
    
    show_prompt();
}
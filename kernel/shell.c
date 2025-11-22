#include "shell.h"
#include "common.h"
#include "fat.h"

char shell_cmd_buf[256];
int shell_cmd_pos = 0;
char shell_username[32] = "user";
int shell_setup_mode = 0;
volatile uint16_t *video = (volatile uint16_t*)0xB8000;
int shell_offset = 0;

// VGA Colors
#define VGA_COLOR_BLACK 0
#define VGA_COLOR_LIGHT_GREY 7
#define VGA_COLOR_LIGHT_ORANGE 6
#define VGA_COLOR_YELLOW 14
#define VGA_COLOR_WHITE 15

// Current attribute (Orange on Black for Bengal Tiger theme)
static uint8_t current_color = VGA_COLOR_YELLOW | (VGA_COLOR_BLACK << 4);

static void scroll() {
    // Basic scrolling logic logic
    if (shell_offset >= 80 * 25) {
        // Move lines up
        for (int i = 0; i < 24 * 80; i++) {
            video[i] = video[i + 80];
        }
        // Clear last line
        for (int i = 24 * 80; i < 25 * 80; i++) {
             video[i] = 0x20 | (current_color << 8);
        }
        shell_offset = 24 * 80;
    }
}

void shell_print_color(const char *s, uint8_t color) {
    int len = strlen(s);
    for (int i = 0; i < len; ++i) {
        if (s[i] == '\n') {
            shell_offset = (shell_offset / 80 + 1) * 80;
        } else {
            video[shell_offset] = (uint16_t)s[i] | ((uint16_t)color << 8);
            shell_offset++;
        }
        scroll();
    }
}

void shell_print(const char *s) {
    shell_print_color(s, current_color);
}

static void show_prompt() {
    shell_print_color(shell_username, VGA_COLOR_LIGHT_ORANGE);
    shell_print_color("@bengal-tiger:~$ ", VGA_COLOR_WHITE);
}

static void clear_screen() {
    for (int i = 0; i < 80 * 25; i++) {
        video[i] = 0x20 | (current_color << 8);
    }
    shell_offset = 0;
}

void shell_handler(char ch) {
    if (shell_setup_mode) {
        if (ch == '\n') {
            shell_cmd_buf[shell_cmd_pos] = 0;
            memcpy(shell_username, shell_cmd_buf, 31);
            shell_setup_mode = 0;
            clear_screen();
            shell_init(shell_username);
        } else {
            shell_cmd_buf[shell_cmd_pos++] = ch;
            char temp[2] = {ch, 0};
            shell_print(temp);
        }
        return;
    }

    if (ch == '\n') {
        shell_cmd_buf[shell_cmd_pos] = 0;
        shell_print("\n");
        
        if (strcmp(shell_cmd_buf, "help") == 0) {
            shell_print("Commands: help, ls, cat <file>, echo <msg>, clear, info, exit\n");
        } else if (strcmp(shell_cmd_buf, "ls") == 0) {
            shell_print("Files: hello.txt config.cfg\n");
        } else if (strncmp(shell_cmd_buf, "cat ", 4) == 0) {
            char *file = shell_cmd_buf + 4;
            char file_buf[512] = {0};
            fat_load_file(file, file_buf);
            shell_print(file_buf);
            shell_print("\n");
        } else if (strncmp(shell_cmd_buf, "echo ", 5) == 0) {
            shell_print(shell_cmd_buf + 5);
            shell_print("\n");
        } else if (strcmp(shell_cmd_buf, "clear") == 0) {
            clear_screen();
        } else if (strcmp(shell_cmd_buf, "info") == 0) {
             shell_print_color("Bengal Tiger OS v0.2\n", VGA_COLOR_LIGHT_ORANGE);
             shell_print("Built for stability and speed.\n");
        } else if (strcmp(shell_cmd_buf, "exit") == 0) {
            shell_print("Halting CPU...\n");
            __asm__ volatile("cli; hlt");
        } else {
            shell_print("Unknown command.\n");
        }
        shell_cmd_pos = 0;
        show_prompt();
    } else if (ch == '\b') {
        // Simple backspace
        if (shell_cmd_pos > 0) {
            shell_cmd_pos--;
            shell_offset--;
            video[shell_offset] = 0x00 | (current_color << 8);
        }
    } else {
        shell_cmd_buf[shell_cmd_pos++] = ch;
        char temp[2] = {ch, 0};
        shell_print(temp);
    }
}

void shell_init(const char *username) {
    memcpy(shell_username, username, 31);
    clear_screen();
    shell_print_color("Bengal Tiger OS Initialized.\n", VGA_COLOR_LIGHT_ORANGE);
    show_prompt();
}
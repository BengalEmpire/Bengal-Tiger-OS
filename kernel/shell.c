// kernel/shell.c (basic CLI)

#include "shell.h"
#include "common.h"
#include "fat.h"

static char cmd_buf[256];
static int cmd_pos = 0;

void shell_handler(char ch) {
    volatile uint16_t *video = (volatile uint16_t*)0xB8000;
    int shell_offset = 240;  // Row 4+

    if (ch == '\n') {
        cmd_buf[cmd_pos] = 0;
        // Parse stub
        if (cmd_pos == 4 && cmd_buf[0] == 'h' && cmd_buf[1] == 'e' && cmd_buf[2] == 'l' && cmd_buf[3] == 'p') {
            const char *help = "Commands: help, ls, cat";
            int i = 0;
            while (help[i]) {
                video[shell_offset + i] = help[i] | (0x07 << 8);
                i++;
            }
        } else if (cmd_pos == 2 && cmd_buf[0] == 'l' && cmd_buf[1] == 's') {
            const char *ls = "Files: hello.txt";
            int i = 0;
            while (ls[i]) {
                video[shell_offset + i] = ls[i] | (0x07 << 8);
                i++;
            }
        } else if (cmd_pos == 3 && cmd_buf[0] == 'c' && cmd_buf[1] == 'a' && cmd_buf[2] == 't') {
            char file_buf[512];
            fat_load_file("hello.txt", file_buf);
            int i = 0;
            while (file_buf[i] && i < 512) {
                video[shell_offset + i] = file_buf[i] | (0x07 << 8);
                i++;
            }
        }
        cmd_pos = 0;
        // Clear buffer
    } else {
        cmd_buf[cmd_pos++] = ch;
        // Echo
        video[shell_offset + cmd_pos - 1] = ch | (0x07 << 8);
    }
}

void shell_init() {
    // Stub
}
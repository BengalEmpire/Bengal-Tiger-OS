// kernel/keyboard.c

#include "keyboard.h"
#include "common.h"
#include "shell.h"  // For shell_handler

static char scan_to_ascii[128] = {
    0, 27, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', '\b',
    '\t', 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n',
    0, 'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'',
    '`', 0, '\\', 'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.',
    '/', 0, '*', 0, ' ', 0, 0,0,0,0,0,0,0,0,0,0,
    // Rest 0
};

void keyboard_handler() {
    uint8_t scancode = inb(0x60);
    if (scancode & 0x80) {
        // Release
    } else {
        char ch = scan_to_ascii[scancode];
        if (ch) {
            shell_handler(ch);  // Pass to shell
        }
    }
}

void keyboard_install() {
    // Handler in IDT
}
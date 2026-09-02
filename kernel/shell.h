/**
 * Bengal Tiger OS - Enhanced Shell Header
 * 
 * @file shell.h
 * @author Bengal Tiger OS (BengalEmpire)
 * @version 0.6.0
 */

#ifndef SHELL_H
#define SHELL_H

#include "common.h"

/* Shell configuration */
#define SHELL_CMD_MAX_LENGTH 256
#define SHELL_HISTORY_SIZE   16

/* Special keys */
#define SPECIAL_KEY_UP       1001
#define SPECIAL_KEY_DOWN     1002
#define SPECIAL_KEY_LEFT     1003
#define SPECIAL_KEY_RIGHT    1004
#define SPECIAL_KEY_HOME     1005
#define SPECIAL_KEY_END      1006
#define SPECIAL_KEY_DELETE   1007

/* Shell state variables */
extern char shell_username[32];
extern int shell_setup_mode;

/**
 * Initialize shell.
 */
void shell_init(const char *username);

/**
 * Handle incoming character from keyboard.
 */
void shell_handler(char ch);

/**
 * Handle special keys (arrows, home, end, etc.).
 */
void shell_handler_special(int key);

/**
 * Execute command string.
 */
void shell_execute_command(const char *cmd);

/**
 * Print text to shell terminal.
 */
void shell_print(const char *s);

/**
 * Print text with VGA color attribute.
 */
void shell_print_color(const char *s, uint8_t color);

/**
 * Print integer to terminal.
 */
void shell_print_int(int num);

/**
 * Print hexadecimal number.
 */
void shell_print_hex(uint32_t num);

/**
 * Clear terminal screen.
 */
void shell_clear_screen(void);

/**
 * Play boot logo animation.
 */
void shell_boot_animation(void);

#endif /* SHELL_H */

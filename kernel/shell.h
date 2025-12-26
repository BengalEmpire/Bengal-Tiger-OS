/**
 * Bengal Tiger OS - Enhanced Shell
 * 
 * Command-line interface with:
 * - Command history (up/down arrows)
 * - Many built-in commands
 * - Colored output
 * - Boot animation
 * 
 * @file shell.h
 * @author Bengal Tiger OS (BengalEmpire)
 * @version 0.3.0
 */

#ifndef SHELL_H
#define SHELL_H

#include "common.h"

/* VGA Colors */
#define VGA_COLOR_BLACK         0
#define VGA_COLOR_BLUE          1
#define VGA_COLOR_GREEN         2
#define VGA_COLOR_CYAN          3
#define VGA_COLOR_RED           4
#define VGA_COLOR_MAGENTA       5
#define VGA_COLOR_BROWN         6
#define VGA_COLOR_LIGHT_GREY    7
#define VGA_COLOR_DARK_GREY     8
#define VGA_COLOR_LIGHT_BLUE    9
#define VGA_COLOR_LIGHT_GREEN   10
#define VGA_COLOR_LIGHT_CYAN    11
#define VGA_COLOR_LIGHT_RED     12
#define VGA_COLOR_LIGHT_MAGENTA 13
#define VGA_COLOR_YELLOW        14
#define VGA_COLOR_WHITE         15

/* Shell constants */
#define SHELL_CMD_MAX_LENGTH    256
#define SHELL_HISTORY_SIZE      16
#define SHELL_MAX_ARGS          16

/* Special key codes for navigation */
#define SPECIAL_KEY_UP          0x100
#define SPECIAL_KEY_DOWN        0x101
#define SPECIAL_KEY_LEFT        0x102
#define SPECIAL_KEY_RIGHT       0x103
#define SPECIAL_KEY_HOME        0x104
#define SPECIAL_KEY_END         0x105
#define SPECIAL_KEY_DELETE      0x106

/* Global shell state */
extern char shell_cmd_buf[SHELL_CMD_MAX_LENGTH];
extern int shell_cmd_pos;
extern char shell_username[32];
extern int shell_setup_mode;
extern int shell_offset;

/**
 * Initialize the shell with username.
 * @param username User's name for prompt
 */
void shell_init(const char *username);

/**
 * Handle keyboard input character.
 * @param ch ASCII character
 */
void shell_handler(char ch);

/**
 * Handle special key (arrows, home, end, etc.)
 * @param key Special key code
 */
void shell_handler_special(int key);

/**
 * Print string to shell with default color.
 * @param s String to print
 */
void shell_print(const char *s);

/**
 * Print string with specific color.
 * @param s String to print
 * @param color VGA color attribute
 */
void shell_print_color(const char *s, uint8_t color);

/**
 * Print integer to shell.
 * @param num Number to print
 */
void shell_print_int(int num);

/**
 * Print hexadecimal value.
 * @param num Value to print
 */
void shell_print_hex(uint32_t num);

/**
 * Clear the screen.
 */
void shell_clear_screen(void);

/**
 * Display boot animation.
 */
void shell_boot_animation(void);

/**
 * Display neofetch-style system info.
 */
void shell_neofetch(void);

/**
 * Parse command into args and execute.
 */
void shell_execute_command(const char *cmd);

#endif /* SHELL_H */
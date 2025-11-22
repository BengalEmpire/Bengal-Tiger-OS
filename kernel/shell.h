#ifndef SHELL_H
#define SHELL_H

#include "common.h"

// Variables shared with other files
extern char shell_cmd_buf[256];
extern int shell_setup_mode;
extern char shell_username[32];

// Functions
void shell_handler(char ch);
void shell_init(const char *username);
void shell_print(const char *s);
void shell_print_color(const char *s, uint8_t color);

#endif
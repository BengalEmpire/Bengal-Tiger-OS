/**
 * Bengal Tiger OS - Interactive Shell, Console & Graphical Workspace
 *
 * Unified console engine rendering simultaneously to VBE linear framebuffers,
 * VGA 80x25 text memory, and COM1 serial terminals, paired with a full
 * real-world command suite.
 *
 * @file shell.h
 * @author Bengal Tiger OS (BengalEmpire)
 * @version 1.0.0
 */

#ifndef SHELL_H
#define SHELL_H

#include "common.h"

/* Low-level Console Grid Primitives (used by shell, editor, top) */
int  console_cols(void);
int  console_rows(void);
void console_clear_cells(uint8_t attr);
void console_put_cell(int col, int row, char ch, uint8_t attr);
void console_set_cursor(int col, int row, int visible);
void console_refresh_topbar(void);

/* Shell Lifecycle & Main Thread Loop */
void shell_init(void);
void shell_run(void) __attribute__((noreturn));
void shell_handle_event(int ev);
void shell_execute_line(const char *line);

/* Output Helpers */
void shell_clear(void);
void shell_putchar(char c);
void shell_print(const char *str);
void shell_print_color(const char *str, uint8_t color);
void shell_print_int(int num);
void shell_print_hex(uint32_t num);
void shell_print_prompt(void);

/* Legacy IRQ-safe compatibility wrapper */
void shell_handler(char key);

#endif /* SHELL_H */

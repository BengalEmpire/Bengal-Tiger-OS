/**
 * Bengal Tiger OS - Built-in Full-Screen Text Editor Implementation
 *
 * @file editor.c
 * @author Bengal Tiger OS (BengalEmpire)
 * @version 1.0.0
 */

#include "editor.h"
#include "shell.h"
#include "fat.h"
#include "keyboard.h"
#include "heap.h"
#include "serial.h"
#include "common.h"

#define EDITOR_MAX_BUF  32768

typedef struct {
    char     filename[32];
    char    *buf;
    uint32_t len;
    uint32_t cursor;      /* Byte offset in buf (0..len) */
    int      row_offset;  /* First visible document line */
    int      col_offset;  /* Horizontal scroll offset */
    int      dirty;
    int      quit_confirm;
    char     status[80];
} editor_state_t;

static void editor_get_line_col(const editor_state_t *ed, uint32_t pos, int *out_line, int *out_col) {
    int line = 0, col = 0;
    for (uint32_t i = 0; i < pos && i < ed->len; i++) {
        if (ed->buf[i] == '\n') {
            line++;
            col = 0;
        } else {
            col++;
        }
    }
    *out_line = line;
    *out_col  = col;
}

static int editor_total_lines(const editor_state_t *ed) {
    int lines = 1;
    for (uint32_t i = 0; i < ed->len; i++) {
        if (ed->buf[i] == '\n') lines++;
    }
    return lines;
}

static uint32_t editor_line_start(const editor_state_t *ed, int target_line) {
    if (target_line <= 0) return 0;
    int line = 0;
    for (uint32_t i = 0; i < ed->len; i++) {
        if (ed->buf[i] == '\n') {
            line++;
            if (line == target_line) return i + 1;
        }
    }
    return ed->len;
}

static uint32_t editor_line_end(const editor_state_t *ed, uint32_t line_start) {
    uint32_t i = line_start;
    while (i < ed->len && ed->buf[i] != '\n') i++;
    return i;
}

static void editor_move_vertical(editor_state_t *ed, int delta_lines) {
    int cur_line = 0, cur_col = 0;
    editor_get_line_col(ed, ed->cursor, &cur_line, &cur_col);

    int total = editor_total_lines(ed);
    int target = cur_line + delta_lines;
    if (target < 0) target = 0;
    if (target >= total) target = total - 1;

    uint32_t start = editor_line_start(ed, target);
    uint32_t end   = editor_line_end(ed, start);
    uint32_t len   = end - start;
    ed->cursor     = start + ((uint32_t)cur_col < len ? (uint32_t)cur_col : len);
}

static void editor_insert_char(editor_state_t *ed, char c) {
    if (ed->len + 1 >= EDITOR_MAX_BUF) {
        strcpy(ed->status, "Buffer full (32KB limit)");
        return;
    }
    memmove(ed->buf + ed->cursor + 1, ed->buf + ed->cursor, ed->len - ed->cursor);
    ed->buf[ed->cursor] = c;
    ed->cursor++;
    ed->len++;
    ed->buf[ed->len] = '\0';
    ed->dirty = 1;
    ed->quit_confirm = 0;
}

static void editor_backspace(editor_state_t *ed) {
    if (ed->cursor == 0) return;
    memmove(ed->buf + ed->cursor - 1, ed->buf + ed->cursor, ed->len - ed->cursor);
    ed->cursor--;
    ed->len--;
    ed->buf[ed->len] = '\0';
    ed->dirty = 1;
    ed->quit_confirm = 0;
}

static void editor_delete(editor_state_t *ed) {
    if (ed->cursor >= ed->len) return;
    memmove(ed->buf + ed->cursor, ed->buf + ed->cursor + 1, ed->len - ed->cursor - 1);
    ed->len--;
    ed->buf[ed->len] = '\0';
    ed->dirty = 1;
    ed->quit_confirm = 0;
}

static void editor_draw_bar(int row, int cols, const char *left, const char *right, uint8_t attr) {
    for (int c = 0; c < cols; c++) {
        console_put_cell(c, row, ' ', attr);
    }
    for (int i = 0; left[i] && i < cols; i++) {
        console_put_cell(i, row, left[i], attr);
    }
    int rlen = strlen(right);
    int rstart = cols - rlen - 1;
    if (rstart < 0) rstart = 0;
    for (int i = 0; right[i] && (rstart + i) < cols; i++) {
        console_put_cell(rstart + i, row, right[i], attr);
    }
}

static void editor_render(editor_state_t *ed) {
    int cols = console_cols();
    int rows = console_rows();
    int body_rows = rows - 2;
    if (body_rows < 1) body_rows = 1;

    int cur_line = 0, cur_col = 0;
    editor_get_line_col(ed, ed->cursor, &cur_line, &cur_col);

    if (cur_line < ed->row_offset) {
        ed->row_offset = cur_line;
    } else if (cur_line >= ed->row_offset + body_rows) {
        ed->row_offset = cur_line - body_rows + 1;
    }

    if (cur_col < ed->col_offset) {
        ed->col_offset = cur_col;
    } else if (cur_col >= ed->col_offset + cols) {
        ed->col_offset = cur_col - cols + 1;
    }

    /* Top Header Bar */
    char top_left[80] = " BENGAL EDIT v1.0 | ";
    strcat(top_left, ed->filename);
    if (ed->dirty) strcat(top_left, " [MODIFIED]");

    char top_right[40] = "Ln ";
    char nbuf[16];
    k_itoa(cur_line + 1, nbuf, 10);
    strcat(top_right, nbuf);
    strcat(top_right, ", Col ");
    k_itoa(cur_col + 1, nbuf, 10);
    strcat(top_right, nbuf);
    editor_draw_bar(0, cols, top_left, top_right, 0x30); /* Black on Cyan */

    /* Document Body */
    int total_lines = editor_total_lines(ed);
    uint32_t pos = editor_line_start(ed, ed->row_offset);

    for (int r = 0; r < body_rows; r++) {
        int doc_line = ed->row_offset + r;
        int screen_row = 1 + r;

        if (doc_line >= total_lines) {
            console_put_cell(0, screen_row, '~', 0x08);
            for (int c = 1; c < cols; c++) {
                console_put_cell(c, screen_row, ' ', 0x07);
            }
        } else {
            uint32_t end = editor_line_end(ed, pos);
            uint32_t line_len = end - pos;

            for (int c = 0; c < cols; c++) {
                uint32_t idx = (uint32_t)(ed->col_offset + c);
                char ch = ' ';
                if (idx < line_len) {
                    ch = ed->buf[pos + idx];
                    if (ch < 32 || ch > 126) ch = '.';
                }
                console_put_cell(c, screen_row, ch, 0x0F);
            }
            pos = (end < ed->len) ? (end + 1) : ed->len;
        }
    }

    /* Bottom Status/Help Bar */
    const char *help = ed->status[0] ? ed->status : " ^S Save   ^Q / ESC Quit   Arrows/PgUp/PgDn Move";
    char bot_right[32];
    k_utoa(ed->len, bot_right, 10);
    strcat(bot_right, " B");
    editor_draw_bar(rows - 1, cols, help, bot_right, 0x70); /* Black on Light Grey */

    int screen_x = cur_col - ed->col_offset;
    int screen_y = 1 + (cur_line - ed->row_offset);
    console_set_cursor(screen_x, screen_y, 1);
    console_refresh_topbar();
}

int editor_open(const char *filename) {
    if (!filename || !filename[0]) return -1;
    if (!fat_is_mounted()) return -1;

    editor_state_t ed;
    memset(&ed, 0, sizeof(ed));
    strncpy(ed.filename, filename, sizeof(ed.filename) - 1);

    ed.buf = (char*)kmalloc(EDITOR_MAX_BUF);
    if (!ed.buf) return -1;
    memset(ed.buf, 0, EDITOR_MAX_BUF);

    uint32_t fsize = 0;
    if (fat_file_exists(filename)) {
        if (fat_read_file(filename, ed.buf, EDITOR_MAX_BUF - 1, &fsize) == 0) {
            ed.len = (fsize < EDITOR_MAX_BUF - 1) ? fsize : (EDITOR_MAX_BUF - 1);
            ed.buf[ed.len] = '\0';
            strcpy(ed.status, " Loaded existing file");
        }
    } else {
        strcpy(ed.status, " [New File] Press ^S to save, ^Q to quit");
    }

    int running = 1;
    while (running) {
        editor_render(&ed);
        int key = input_wait_event();
        ed.status[0] = '\0';

        if (key == CTRL_KEY('s')) {
            if (fat_write_file(ed.filename, ed.buf, ed.len) == 0) {
                ed.dirty = 0;
                ed.quit_confirm = 0;
                char nbuf[16];
                k_utoa(ed.len, nbuf, 10);
                strcpy(ed.status, " Saved ");
                strcat(ed.status, nbuf);
                strcat(ed.status, " bytes to ");
                strcat(ed.status, ed.filename);
            } else {
                strcpy(ed.status, " ERROR: Failed to save file!");
            }
        } else if (key == CTRL_KEY('q') || key == 27) {
            if (ed.dirty && !ed.quit_confirm) {
                ed.quit_confirm = 1;
                strcpy(ed.status, " Unsaved changes! Press ^Q/ESC again to discard, or ^S to save.");
            } else {
                running = 0;
            }
        } else if (key == KEY_SPECIAL_LEFT) {
            if (ed.cursor > 0) ed.cursor--;
        } else if (key == KEY_SPECIAL_RIGHT) {
            if (ed.cursor < ed.len) ed.cursor++;
        } else if (key == KEY_SPECIAL_UP) {
            editor_move_vertical(&ed, -1);
        } else if (key == KEY_SPECIAL_DOWN) {
            editor_move_vertical(&ed, 1);
        } else if (key == KEY_SPECIAL_PGUP) {
            editor_move_vertical(&ed, -(console_rows() - 3));
        } else if (key == KEY_SPECIAL_PGDN) {
            editor_move_vertical(&ed, (console_rows() - 3));
        } else if (key == KEY_SPECIAL_HOME) {
            int l = 0, c = 0;
            editor_get_line_col(&ed, ed.cursor, &l, &c);
            ed.cursor = editor_line_start(&ed, l);
        } else if (key == KEY_SPECIAL_END) {
            int l = 0, c = 0;
            editor_get_line_col(&ed, ed.cursor, &l, &c);
            ed.cursor = editor_line_end(&ed, editor_line_start(&ed, l));
        } else if (key == '\b') {
            editor_backspace(&ed);
        } else if (key == KEY_SPECIAL_DELETE) {
            editor_delete(&ed);
        } else if (key == '\n') {
            editor_insert_char(&ed, '\n');
        } else if (key == '\t') {
            for (int i = 0; i < 4; i++) editor_insert_char(&ed, ' ');
        } else if (key >= 32 && key <= 126) {
            editor_insert_char(&ed, (char)key);
        }
    }

    kfree(ed.buf);
    shell_clear();
    return 0;
}

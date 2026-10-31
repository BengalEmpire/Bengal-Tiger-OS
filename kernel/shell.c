/**
 * Bengal Tiger OS - Interactive Shell, Unified Console & GUI Implementation
 *
 * @file shell.c
 * @author Bengal Tiger OS (BengalEmpire)
 * @version 1.0.0
 */

#include "shell.h"
#include "version.h"
#include "common.h"
#include "vbe.h"
#include "font.h"
#include "serial.h"
#include "keyboard.h"
#include "mouse.h"
#include "cpu.h"
#include "rtc.h"
#include "timer.h"
#include "paging.h"
#include "heap.h"
#include "scheduler.h"
#include "disk.h"
#include "fat.h"
#include "pci.h"
#include "nic.h"
#include "net.h"
#include "editor.h"
#include "klog.h"

/* =========================================================================
 * Unified Console State (VBE Graphical + VGA 80x25 + Serial COM1)
 * ========================================================================= */
#define VGA_MEMORY          ((volatile uint16_t*)0xB8000)
#define MAX_GRID_COLS       200
#define MAX_GRID_ROWS       100
#define VBE_TOPBAR_HEIGHT   28

typedef struct {
    char    ch;
    uint8_t attr;
} console_cell_t;

static console_cell_t shadow_grid[MAX_GRID_ROWS][MAX_GRID_COLS];
static int con_cols = 80;
static int con_rows = 25;
static int cur_x    = 0;
static int cur_y    = 0;
static int drawn_cur_x = -1;
static int drawn_cur_y = -1;
static int cur_visible = 1;
static uint8_t default_attr = 0x07;

/* Modern Bengal Tiger OS 16-Color RGB Palette for VBE Mode */
static const uint32_t shell_theme_palette[16] = {
    0x000D1117, /* 0x0 Deep Obsidian Black */
    0x001F6FEB, /* 0x1 Ocean Blue          */
    0x002EA043, /* 0x2 Jungle Green        */
    0x0039C5CF, /* 0x3 Teal Cyan           */
    0x00DA3633, /* 0x4 Crimson Red         */
    0x008957E5, /* 0x5 Royal Purple        */
    0x00D29922, /* 0x6 Tiger Amber         */
    0x00C9D1D9, /* 0x7 Soft Silver         */
    0x006E7681, /* 0x8 Slate Grey          */
    0x0058A6FF, /* 0x9 Sky Blue            */
    0x003FB950, /* 0xA Emerald Green       */
    0x0056D4DD, /* 0xB Bright Cyan         */
    0x00F85149, /* 0xC Coral Red           */
    0x00BC8CFF, /* 0xD Soft Violet         */
    0x00FFA657, /* 0xE Bengal Tiger Gold   */
    0x00F0F6FC  /* 0xF Crisp White         */
};

static inline void ser_putc(char c) {
    serial_write_char(COM1_PORT, c);
}

static inline void ser_puts(const char *s) {
    serial_write_str(s);
}

static void vga_hw_set_cursor(int x, int y, int visible) {
    if (!visible) {
        outb(0x3D4, 0x0A);
        outb(0x3D5, 0x20);
        return;
    }
    outb(0x3D4, 0x0A);
    outb(0x3D5, (uint8_t)((inb(0x3D5) & 0xC0) | 13));
    outb(0x3D4, 0x0B);
    outb(0x3D5, (uint8_t)((inb(0x3D5) & 0xE0) | 15));

    uint16_t pos = (uint16_t)(y * 80 + x);
    outb(0x3D4, 0x0F);
    outb(0x3D5, (uint8_t)(pos & 0xFF));
    outb(0x3D4, 0x0E);
    outb(0x3D5, (uint8_t)((pos >> 8) & 0xFF));
}

static void vbe_render_cell_raw(int col, int row, char ch, uint8_t attr) {
    uint32_t fg = shell_theme_palette[attr & 0x0F];
    uint32_t bg = shell_theme_palette[(attr >> 4) & 0x0F];
    uint32_t px = (uint32_t)(col * FONT_WIDTH);
    uint32_t py = (uint32_t)(VBE_TOPBAR_HEIGHT + row * FONT_HEIGHT);
    vbe_draw_char(px, py, ch ? ch : ' ', fg, bg);
}

static void vbe_undraw_cursor(void) {
    if (!vbe_is_active()) return;
    if (drawn_cur_x >= 0 && drawn_cur_x < con_cols &&
        drawn_cur_y >= 0 && drawn_cur_y < con_rows) {
        console_cell_t cell = shadow_grid[drawn_cur_y][drawn_cur_x];
        vbe_render_cell_raw(drawn_cur_x, drawn_cur_y, cell.ch, cell.attr);
    }
    drawn_cur_x = -1;
    drawn_cur_y = -1;
}

static void vbe_draw_cursor_at(int col, int row) {
    if (!vbe_is_active() || !cur_visible) return;
    if (col < 0 || col >= con_cols || row < 0 || row >= con_rows) return;

    console_cell_t cell = shadow_grid[row][col];
    uint32_t px = (uint32_t)(col * FONT_WIDTH);
    uint32_t py = (uint32_t)(VBE_TOPBAR_HEIGHT + row * FONT_HEIGHT);
    vbe_draw_char(px, py, cell.ch ? cell.ch : ' ', 0x000D1117, 0x00FFA657);
    drawn_cur_x = col;
    drawn_cur_y = row;
}

int console_cols(void) { return con_cols; }
int console_rows(void) { return con_rows; }

void console_refresh_topbar(void) {
    if (!vbe_is_active()) return;

    uint32_t w = vbe_fb.width;
    vbe_fill_rect(0, 0, w, VBE_TOPBAR_HEIGHT - 2, 0x00161B22);
    vbe_fill_rect(0, VBE_TOPBAR_HEIGHT - 2, w, 2, 0x00FF7B00);

    vbe_draw_string(12, 6, "BENGAL TIGER OS v" OS_VERSION, 0x00FFA657, 0x00161B22);

    /* Center: Network / Storage indicator */
    char mid[64] = "";
    if (net_config.configured) {
        char ip_s[20];
        net_format_ip(net_config.ip, ip_s);
        strcpy(mid, "NET: ");
        strcat(mid, ip_s);
    } else {
        strcpy(mid, "NET: offline");
    }
    strcat(mid, disk_is_ramdisk() ? "  |  FS: RAMDisk" : "  |  FS: ATA HDD");
    int mid_x = ((int)w - strlen(mid) * FONT_WIDTH) / 2;
    if (mid_x > 220) {
        vbe_draw_string((uint32_t)mid_x, 6, mid, 0x008B949E, 0x00161B22);
    }

    /* Right: Live UTC RTC Clock */
    rtc_time_t rt;
    rtc_read_time(&rt);
    char clk[20];
    rtc_format_time(&rt, clk);
    strcat(clk, " UTC");

    int rx = (int)w - strlen(clk) * FONT_WIDTH - 12;
    if (rx > 0) {
        vbe_draw_string((uint32_t)rx, 6, clk, 0x0056D4DD, 0x00161B22);
    }
}

void console_put_cell(int col, int row, char ch, uint8_t attr) {
    if (col < 0 || col >= con_cols || row < 0 || row >= con_rows) return;

    shadow_grid[row][col].ch   = ch;
    shadow_grid[row][col].attr = attr;

    if (vbe_is_active()) {
        if (col == drawn_cur_x && row == drawn_cur_y) {
            drawn_cur_x = -1;
            drawn_cur_y = -1;
        }
        vbe_render_cell_raw(col, row, ch, attr);
    } else {
        VGA_MEMORY[row * 80 + col] = (uint16_t)((uint8_t)ch) | ((uint16_t)attr << 8);
    }
}

void console_set_cursor(int col, int row, int visible) {
    if (col < 0) col = 0;
    if (col >= con_cols) col = con_cols - 1;
    if (row < 0) row = 0;
    if (row >= con_rows) row = con_rows - 1;

    cur_x = col;
    cur_y = row;
    cur_visible = visible;

    if (vbe_is_active()) {
        vbe_undraw_cursor();
        if (visible) {
            vbe_draw_cursor_at(col, row);
        }
    } else {
        vga_hw_set_cursor(col, row, visible);
    }
}

void console_clear_cells(uint8_t attr) {
    drawn_cur_x = -1;
    drawn_cur_y = -1;

    for (int r = 0; r < con_rows; r++) {
        for (int c = 0; c < con_cols; c++) {
            shadow_grid[r][c].ch   = ' ';
            shadow_grid[r][c].attr = attr;
        }
    }

    if (vbe_is_active()) {
        uint32_t bg = shell_theme_palette[(attr >> 4) & 0x0F];
        vbe_fill_rect(0, VBE_TOPBAR_HEIGHT,
                      vbe_fb.width,
                      vbe_fb.height - VBE_TOPBAR_HEIGHT, bg);
        console_refresh_topbar();
    } else {
        uint16_t blank = (uint16_t)' ' | ((uint16_t)attr << 8);
        for (int i = 0; i < 80 * 25; i++) {
            VGA_MEMORY[i] = blank;
        }
    }
    console_set_cursor(0, 0, 1);
}

static void console_scroll_up(void) {
    if (vbe_is_active()) {
        vbe_undraw_cursor();
    }

    for (int r = 0; r < con_rows - 1; r++) {
        memcpy(shadow_grid[r], shadow_grid[r + 1], (uint32_t)con_cols * sizeof(console_cell_t));
    }
    for (int c = 0; c < con_cols; c++) {
        shadow_grid[con_rows - 1][c].ch   = ' ';
        shadow_grid[con_rows - 1][c].attr = default_attr;
    }

    if (vbe_is_active()) {
        uint32_t region_h = (uint32_t)(con_rows * FONT_HEIGHT);
        vbe_scroll_region(VBE_TOPBAR_HEIGHT, region_h, FONT_HEIGHT, shell_theme_palette[0]);
    } else {
        for (int i = 0; i < 80 * 24; i++) {
            VGA_MEMORY[i] = VGA_MEMORY[i + 80];
        }
        uint16_t blank = (uint16_t)' ' | ((uint16_t)default_attr << 8);
        for (int c = 0; c < 80; c++) {
            VGA_MEMORY[24 * 80 + c] = blank;
        }
    }
}

static void console_write_char_color(char c, uint8_t color, int mirror_serial) {
    if (mirror_serial) {
        if (c == '\b') {
            ser_puts("\b \b");
        } else {
            ser_putc(c);
        }
    }

    if (vbe_is_active()) {
        vbe_undraw_cursor();
    }

    if (c == '\n') {
        cur_x = 0;
        cur_y++;
    } else if (c == '\r') {
        cur_x = 0;
    } else if (c == '\t') {
        int spaces = 4 - (cur_x & 3);
        for (int i = 0; i < spaces; i++) {
            console_write_char_color(' ', color, 0);
        }
        return;
    } else if (c == '\b') {
        if (cur_x > 0) {
            cur_x--;
        } else if (cur_y > 0) {
            cur_y--;
            cur_x = con_cols - 1;
        }
        console_put_cell(cur_x, cur_y, ' ', default_attr);
    } else if ((uint8_t)c >= 32) {
        console_put_cell(cur_x, cur_y, c, color);
        cur_x++;
        if (cur_x >= con_cols) {
            cur_x = 0;
            cur_y++;
        }
    }

    while (cur_y >= con_rows) {
        console_scroll_up();
        cur_y--;
    }

    console_set_cursor(cur_x, cur_y, 1);
}

void shell_clear(void) {
    console_clear_cells(default_attr);
    ser_puts("\x1B[2J\x1B[H");
}

void shell_putchar(char c) {
    console_write_char_color(c, default_attr, 1);
}

void shell_print(const char *str) {
    if (!str) return;
    while (*str) {
        console_write_char_color(*str++, default_attr, 1);
    }
}

void shell_print_color(const char *str, uint8_t color) {
    if (!str) return;
    while (*str) {
        console_write_char_color(*str++, color, 1);
    }
}

void shell_print_int(int num) {
    char buf[16];
    k_itoa(num, buf, 10);
    shell_print(buf);
}

void shell_print_hex(uint32_t num) {
    char buf[16];
    shell_print("0x");
    k_utoa(num, buf, 16);
    if (strlen(buf) < 2) shell_print("0");
    shell_print(buf);
}

static void print_hex_pad(uint32_t val, int digits) {
    const char *hx = "0123456789ABCDEF";
    char out[12];
    for (int i = digits - 1; i >= 0; i--) {
        out[i] = hx[val & 0xF];
        val >>= 4;
    }
    out[digits] = '\0';
    shell_print(out);
}

/* =========================================================================
 * Shell Line Buffer, History & Tab Completion State
 * ========================================================================= */
#define CMD_BUF_MAX     256
#define HISTORY_MAX     16

static char username[32] = "tiger";
static char cmd_buf[CMD_BUF_MAX];
static int  cmd_len     = 0;
static int  cmd_cursor  = 0;
static int  prompt_col  = 0;
static int  prompt_row  = 0;

static char history[HISTORY_MAX][CMD_BUF_MAX];
static int  history_count = 0;
static int  history_nav   = -1;
static int  script_depth  = 0;

static const char *builtin_cmds[] = {
    "help", "info", "neofetch", "ver", "cpu", "date", "uptime", "dmesg",
    "user", "history", "echo", "calc", "beep", "gui", "clear", "reboot",
    "shutdown", "poweroff", "mem", "ps", "top", "spawn", "kill",
    "ls", "dir", "cat", "write", "append", "edit", "cp", "mv", "rm",
    "touch", "stat", "hexdump", "sh", "df", "disk", "mkfs",
    "lspci", "ifconfig", "dhcp", "ping", "arp", "netstat"
};

void shell_print_prompt(void) {
    shell_print_color(username, 0x0E);       /* Tiger Gold */
    shell_print_color("@bengal", 0x0A);      /* Jungle Green */
    shell_print_color(":", 0x07);
    shell_print_color("~", 0x0B);            /* Cyan */
    shell_print_color("$ ", 0x0F);           /* White */
    prompt_col = cur_x;
    prompt_row = cur_y;
}

static void redraw_input_line(int prev_len) {
    int max_draw = (cmd_len > prev_len) ? cmd_len : prev_len;

    if (vbe_is_active()) {
        vbe_undraw_cursor();
    }

    int total_cells = prompt_col + max_draw + 1;
    int needed_rows = (total_cells + con_cols - 1) / con_cols;
    while (prompt_row + needed_rows > con_rows) {
        console_scroll_up();
        prompt_row--;
    }

    for (int i = 0; i < max_draw; i++) {
        int pos = prompt_col + i;
        int r = prompt_row + (pos / con_cols);
        int c = pos % con_cols;
        char ch = (i < cmd_len) ? cmd_buf[i] : ' ';
        console_put_cell(c, r, ch, 0x0F);
    }

    int cpos = prompt_col + cmd_cursor;
    console_set_cursor(cpos % con_cols, prompt_row + (cpos / con_cols), 1);

    /* Keep serial terminal cleanly synchronized */
    ser_putc('\r');
    ser_puts(username);
    ser_puts("@bengal:~$ ");
    for (int i = 0; i < cmd_len; i++) ser_putc(cmd_buf[i]);
    for (int i = cmd_len; i < prev_len; i++) ser_putc(' ');
    int back = ((prev_len > cmd_len) ? prev_len : cmd_len) - cmd_cursor;
    for (int i = 0; i < back; i++) ser_putc('\b');
}

static void history_push(const char *line) {
    if (!line[0]) return;
    if (history_count > 0 && strcmp(history[history_count - 1], line) == 0) {
        return;
    }
    if (history_count < HISTORY_MAX) {
        strcpy(history[history_count++], line);
    } else {
        for (int i = 0; i < HISTORY_MAX - 1; i++) {
            strcpy(history[i], history[i + 1]);
        }
        strcpy(history[HISTORY_MAX - 1], line);
    }
}

static void handle_tab_completion(void) {
    int tok_start = cmd_cursor;
    while (tok_start > 0 && cmd_buf[tok_start - 1] != ' ') {
        tok_start--;
    }
    int prefix_len = cmd_cursor - tok_start;
    char prefix[64];
    if (prefix_len >= (int)sizeof(prefix)) return;
    memcpy(prefix, cmd_buf + tok_start, (uint32_t)prefix_len);
    prefix[prefix_len] = '\0';

    int is_first_token = 1;
    for (int i = 0; i < tok_start; i++) {
        if (cmd_buf[i] != ' ') { is_first_token = 0; break; }
    }

    char matches[48][16];
    int match_count = 0;

    if (is_first_token) {
        for (unsigned int i = 0; i < ARRAY_SIZE(builtin_cmds) && match_count < 48; i++) {
            if (strncasecmp(builtin_cmds[i], prefix, (uint32_t)prefix_len) == 0) {
                strncpy(matches[match_count++], builtin_cmds[i], 15);
                matches[match_count - 1][15] = '\0';
            }
        }
    } else {
        char files[48][16];
        int nf = fat_enumerate_files(files, 48);
        for (int i = 0; i < nf && match_count < 48; i++) {
            if (strncasecmp(files[i], prefix, (uint32_t)prefix_len) == 0) {
                int k = 0;
                for (; files[i][k] && k < 15; k++) {
                    char ch = files[i][k];
                    if (ch >= 'A' && ch <= 'Z') ch = (char)(ch - 'A' + 'a');
                    matches[match_count][k] = ch;
                }
                matches[match_count][k] = '\0';
                match_count++;
            }
        }
    }

    if (match_count == 0) return;

    int prev_len = cmd_len;
    if (match_count == 1) {
        const char *m = matches[0];
        int mlen = strlen(m);
        int add = mlen - prefix_len;
        if (cmd_len + add + 2 < CMD_BUF_MAX) {
            memmove(cmd_buf + cmd_cursor + add, cmd_buf + cmd_cursor, (uint32_t)(cmd_len - cmd_cursor + 1));
            memcpy(cmd_buf + tok_start, m, (uint32_t)mlen);
            cmd_len    += add;
            cmd_cursor += add;
            if (cmd_cursor == cmd_len && cmd_len + 1 < CMD_BUF_MAX) {
                cmd_buf[cmd_len++] = ' ';
                cmd_buf[cmd_len]   = '\0';
                cmd_cursor++;
            }
            redraw_input_line(prev_len);
        }
    } else {
        int common = strlen(matches[0]);
        for (int i = 1; i < match_count; i++) {
            int j = 0;
            while (j < common && matches[0][j] == matches[i][j]) j++;
            common = j;
        }
        if (common > prefix_len && cmd_len + (common - prefix_len) + 1 < CMD_BUF_MAX) {
            int add = common - prefix_len;
            memmove(cmd_buf + cmd_cursor + add, cmd_buf + cmd_cursor, (uint32_t)(cmd_len - cmd_cursor + 1));
            memcpy(cmd_buf + tok_start, matches[0], (uint32_t)common);
            cmd_len    += add;
            cmd_cursor += add;
        }

        shell_putchar('\n');
        for (int i = 0; i < match_count; i++) {
            shell_print_color(matches[i], 0x0B);
            shell_print("  ");
        }
        shell_putchar('\n');
        shell_print_prompt();
        redraw_input_line(0);
    }
}

/* =========================================================================
 * Recursive-Descent Integer Calculator (`calc`)
 * ========================================================================= */
typedef struct {
    const char *s;
    int pos;
    int err;
} calc_ctx_t;

static int calc_parse_expr(calc_ctx_t *c);

static void calc_skip_ws(calc_ctx_t *c) {
    while (c->s[c->pos] == ' ' || c->s[c->pos] == '\t') c->pos++;
}

static int calc_parse_factor(calc_ctx_t *c) {
    calc_skip_ws(c);
    if (c->s[c->pos] == '+') { c->pos++; return calc_parse_factor(c); }
    if (c->s[c->pos] == '-') { c->pos++; return -calc_parse_factor(c); }
    if (c->s[c->pos] == '(') {
        c->pos++;
        int v = calc_parse_expr(c);
        calc_skip_ws(c);
        if (c->s[c->pos] == ')') c->pos++;
        else c->err = 1;
        return v;
    }

    int val = 0;
    int digits = 0;
    if (c->s[c->pos] == '0' && (c->s[c->pos + 1] == 'x' || c->s[c->pos + 1] == 'X')) {
        c->pos += 2;
        while (1) {
            char ch = c->s[c->pos];
            int d = -1;
            if (ch >= '0' && ch <= '9') d = ch - '0';
            else if (ch >= 'a' && ch <= 'f') d = 10 + (ch - 'a');
            else if (ch >= 'A' && ch <= 'F') d = 10 + (ch - 'A');
            else break;
            val = (val << 4) | d;
            c->pos++;
            digits++;
        }
    } else {
        while (c->s[c->pos] >= '0' && c->s[c->pos] <= '9') {
            val = val * 10 + (c->s[c->pos] - '0');
            c->pos++;
            digits++;
        }
    }
    if (!digits) c->err = 1;
    return val;
}

static int calc_parse_term(calc_ctx_t *c) {
    int left = calc_parse_factor(c);
    while (!c->err) {
        calc_skip_ws(c);
        char op = c->s[c->pos];
        if (op != '*' && op != '/' && op != '%') break;
        c->pos++;
        int right = calc_parse_factor(c);
        if (op == '*') left *= right;
        else {
            if (right == 0) { c->err = 2; return 0; }
            if (op == '/') left /= right;
            else left %= right;
        }
    }
    return left;
}

static int calc_parse_expr(calc_ctx_t *c) {
    int left = calc_parse_term(c);
    while (!c->err) {
        calc_skip_ws(c);
        char op = c->s[c->pos];
        if (op != '+' && op != '-') break;
        c->pos++;
        int right = calc_parse_term(c);
        if (op == '+') left += right;
        else left -= right;
    }
    return left;
}

/* =========================================================================
 * Preemptive Background Worker Demo (`spawn`)
 * ========================================================================= */
static volatile uint32_t bg_worker_counter = 0;

static void bg_worker_entry(void) {
    for (int iter = 0; iter < 600; iter++) {
        bg_worker_counter++;
        sleep_ms(50);
    }
}

/* =========================================================================
 * Built-in Graphical Desktop Workspace (`gui`)
 * ========================================================================= */
static void gui_draw_card(uint32_t x, uint32_t y, uint32_t w, uint32_t h, const char *title, uint32_t accent) {
    vbe_fill_rect(x + 4, y + 4, w, h, 0x00070A0F); /* Drop shadow */
    vbe_fill_rect(x, y, w, h, 0x00161B22);
    vbe_fill_rect(x, y, w, 26, accent);
    vbe_draw_rect(x, y, w, h, 0x0030363D, 1);
    vbe_draw_string(x + 10, y + 5, title, 0x000D1117, accent);
}

static void cmd_gui(void) {
    if (!vbe_is_active()) {
        shell_print_color("VBE graphical framebuffer is not active in this boot mode.\n", 0x0C);
        shell_print("Select 'Bengal Tiger OS v" OS_VERSION "' (default) in GRUB for 1024x768x32 graphics.\n");
        return;
    }

    uint32_t sw = vbe_fb.width;
    uint32_t sh = vbe_fb.height;

    char ping_msg[64] = "Click [Ping Gateway] or press 'P' to test ICMP";
    uint8_t prev_btn = 0;
    int32_t last_mx = -1, last_my = -1;
    uint32_t last_redraw_tick = 0;

    int full_redraw = 1;

    while (1) {
        int ev = input_poll_event();
        if (ev == 27 || ev == 'q' || ev == 'Q' || ev == CTRL_KEY('c')) {
            break;
        }
        if (ev == 'p' || ev == 'P') {
            uint32_t gw = net_config.gateway ? net_config.gateway : net_make_ip(10, 0, 2, 2);
            uint32_t rtt = 0;
            uint8_t ttl = 0;
            if (net_ping(gw, 1, 800, &rtt, &ttl) == 0) {
                char ip_s[20], r_s[16];
                net_format_ip(gw, ip_s);
                k_utoa(rtt, r_s, 10);
                strcpy(ping_msg, "Reply from ");
                strcat(ping_msg, ip_s);
                strcat(ping_msg, " time=");
                strcat(ping_msg, r_s);
                strcat(ping_msg, "ms");
            } else {
                strcpy(ping_msg, "Ping timed out / host unreachable");
            }
            full_redraw = 1;
        }

        nic_poll();
        int32_t mx = mouse_get_x();
        int32_t my = mouse_get_y();
        uint8_t btns = mouse_get_buttons();

        int32_t btn_exit_x = (int32_t)sw - 160, btn_exit_y = 4, btn_exit_w = 148, btn_exit_h = 22;
        int32_t card_net_x = 32, card_net_y = 240;
        int32_t btn_ping_x = card_net_x + 16, btn_ping_y = card_net_y + 132, btn_ping_w = 156, btn_ping_h = 24;

        int left_click = ((btns & MOUSE_BTN_LEFT) && !(prev_btn & MOUSE_BTN_LEFT));
        prev_btn = btns;

        if (left_click) {
            if (mx >= btn_exit_x && mx < btn_exit_x + btn_exit_w &&
                my >= btn_exit_y && my < btn_exit_y + btn_exit_h) {
                break;
            }
            if (mx >= btn_ping_x && mx < btn_ping_x + btn_ping_w &&
                my >= btn_ping_y && my < btn_ping_y + btn_ping_h) {
                uint32_t gw = net_config.gateway ? net_config.gateway : net_make_ip(10, 0, 2, 2);
                uint32_t rtt = 0; uint8_t ttl = 0;
                if (net_ping(gw, 1, 800, &rtt, &ttl) == 0) {
                    char ip_s[20], r_s[16];
                    net_format_ip(gw, ip_s);
                    k_utoa(rtt, r_s, 10);
                    strcpy(ping_msg, "Reply from ");
                    strcat(ping_msg, ip_s);
                    strcat(ping_msg, " time=");
                    strcat(ping_msg, r_s);
                    strcat(ping_msg, "ms");
                } else {
                    strcpy(ping_msg, "Ping timed out");
                }
                full_redraw = 1;
            }
        }

        uint32_t now = timer_get_ticks();
        if (full_redraw || (now - last_redraw_tick) >= 50 || mx != last_mx || my != last_my) {
            if (full_redraw) {
                vbe_fill_rect(0, 0, sw, sh, 0x000F172A);
                vbe_fill_rect(0, 0, sw, 30, 0x00161B22);
                vbe_fill_rect(0, 30, sw, 2, 0x00FF7B00);
                vbe_draw_string(12, 7, "Bengal Tiger OS Desktop v" OS_VERSION, 0x00FFA657, 0x00161B22);

                vbe_fill_rect((uint32_t)btn_exit_x, (uint32_t)btn_exit_y, (uint32_t)btn_exit_w, (uint32_t)btn_exit_h, 0x00DA3633);
                vbe_draw_string((uint32_t)(btn_exit_x + 10), (uint32_t)(btn_exit_y + 3), "Exit GUI [ESC]", 0x00FFFFFF, 0x00DA3633);

                vbe_fill_rect(0, sh - 28, sw, 28, 0x00161B22);
                vbe_draw_string(12, sh - 22, "Mouse: Move & Click  |  [P] Ping Gateway  |  [ESC / Q] Return to Shell",
                                0x008B949E, 0x00161B22);
                full_redraw = 0;
            }

            /* Card 1: System & Memory Monitor */
            gui_draw_card(32, 52, 440, 168, "SYSTEM & MEMORY MONITOR", 0x00FFA657);
            vbe_draw_string(48, 88, "CPU: ", 0x008B949E, 0x00161B22);
            vbe_draw_string(96, 88, cpu_get_brand()[0] ? cpu_get_brand() : cpu_get_vendor(), 0x00F0F6FC, 0x00161B22);

            heap_stats_t hs;
            heap_get_stats(&hs);
            int tcnt = 0;
            (void)scheduler_get_task_list(&tcnt);

            char mbuf[64], nbuf[20];
            strcpy(mbuf, "Physical Free: ");
            k_utoa(pmm_get_free_frames() * 4, nbuf, 10);
            strcat(mbuf, nbuf);
            strcat(mbuf, " KB  |  Tasks: ");
            k_itoa(tcnt, nbuf, 10);
            strcat(mbuf, nbuf);
            vbe_draw_string(48, 114, mbuf, 0x0056D4DD, 0x00161B22);

            strcpy(mbuf, "Kernel Heap Used: ");
            k_utoa(hs.used_size / 1024, nbuf, 10);
            strcat(mbuf, nbuf);
            strcat(mbuf, " KB / ");
            k_utoa(hs.total_size / 1024, nbuf, 10);
            strcat(mbuf, nbuf);
            strcat(mbuf, " KB");
            vbe_draw_string(48, 140, mbuf, 0x003FB950, 0x00161B22);

            strcpy(mbuf, "Uptime: ");
            k_utoa(timer_get_seconds(), nbuf, 10);
            strcat(mbuf, nbuf);
            strcat(mbuf, " sec     ");
            vbe_draw_string(48, 168, mbuf, 0x00C9D1D9, 0x00161B22);

            /* Card 2: Network Stack */
            gui_draw_card((uint32_t)card_net_x, (uint32_t)card_net_y, 440, 174, "NETWORK STACK (TCP/IP)", 0x0056D4DD);
            strcpy(mbuf, "Adapter: ");
            strcat(mbuf, nic_state.driver_name);
            vbe_draw_string((uint32_t)(card_net_x + 16), (uint32_t)(card_net_y + 36), mbuf, 0x00F0F6FC, 0x00161B22);

            char ip_s[20];
            net_format_ip(net_config.ip, ip_s);
            strcpy(mbuf, "IPv4: ");
            strcat(mbuf, ip_s);
            strcat(mbuf, "  |  RX/TX: ");
            k_utoa(nic_state.rx_packets, nbuf, 10);
            strcat(mbuf, nbuf);
            strcat(mbuf, "/");
            k_utoa(nic_state.tx_packets, nbuf, 10);
            strcat(mbuf, nbuf);
            vbe_draw_string((uint32_t)(card_net_x + 16), (uint32_t)(card_net_y + 62), mbuf, 0x003FB950, 0x00161B22);
            vbe_draw_string((uint32_t)(card_net_x + 16), (uint32_t)(card_net_y + 92), ping_msg, 0x00FFA657, 0x00161B22);

            vbe_fill_rect((uint32_t)btn_ping_x, (uint32_t)btn_ping_y, (uint32_t)btn_ping_w, (uint32_t)btn_ping_h, 0x001F6FEB);
            vbe_draw_string((uint32_t)(btn_ping_x + 12), (uint32_t)(btn_ping_y + 4), "[ Ping Gateway ]", 0x00FFFFFF, 0x001F6FEB);

            /* Card 3: Storage Explorer */
            if (sw >= 900) {
                uint32_t cx = 500, cy = 52;
                gui_draw_card(cx, cy, sw - cx - 32, 362, "FAT FILESYSTEM EXPLORER", 0x003FB950);
                fat_fs_info_t fi;
                fat_get_fs_info(&fi);
                strcpy(mbuf, "Volume: ");
                strcat(mbuf, fi.label[0] ? fi.label : "BENGAL");
                strcat(mbuf, fi.is_ramdisk ? " (Live RAMDisk)" : " (ATA Disk)");
                vbe_draw_string(cx + 16, cy + 36, mbuf, 0x00F0F6FC, 0x00161B22);

                char files[12][16];
                int nf = fat_enumerate_files(files, 12);
                for (int i = 0; i < nf; i++) {
                    fat_file_t st;
                    char line[48];
                    strcpy(line, "  * ");
                    strcat(line, files[i]);
                    if (fat_stat_file(files[i], &st) == 0) {
                        for (int k = strlen(line); k < 22; k++) strcat(line, " ");
                        k_utoa(st.size, nbuf, 10);
                        strcat(line, nbuf);
                        strcat(line, " B");
                    }
                    vbe_draw_string(cx + 16, cy + 72 + (uint32_t)(i * 22), line, 0x00C9D1D9, 0x00161B22);
                }
            }

            /* Draw crisp mouse cursor */
            uint32_t cmx = (mx < 0) ? 0 : ((uint32_t)mx >= sw - 10 ? sw - 10 : (uint32_t)mx);
            uint32_t cmy = (my < 0) ? 0 : ((uint32_t)my >= sh - 10 ? sh - 10 : (uint32_t)my);
            vbe_fill_rect(cmx, cmy, 10, 10, 0x00FFA657);
            vbe_draw_rect(cmx, cmy, 10, 10, 0x00FFFFFF, 1);

            if (last_mx != mx || last_my != my) {
                full_redraw = 1;
            }
            last_mx = mx;
            last_my = my;
            last_redraw_tick = now;
        }

        if (irq_are_enabled()) hlt();
    }

    shell_clear();
    shell_print_color("Returned from Graphical Desktop Workspace.\n", 0x0A);
}

/* =========================================================================
 * Shell Built-in Commands Implementation
 * ========================================================================= */
static void cmd_help(void) {
    shell_print_color("\n=== Bengal Tiger OS v" OS_VERSION " Command Reference ===\n", 0x0E);

    shell_print_color("  System & Info:\n", 0x0B);
    shell_print("    help, info (neofetch), ver, cpu, date, uptime, dmesg, user [name]\n");
    shell_print("    history, echo <txt> [> file], calc <expr>, beep [hz] [ms], gui\n");
    shell_print("    clear, reboot, shutdown (poweroff)\n");

    shell_print_color("  Memory & Multitasking:\n", 0x0B);
    shell_print("    mem          - Physical PMM & dynamic kernel heap usage\n");
    shell_print("    ps / top     - Task table or live interactive process monitor\n");
    shell_print("    spawn [n]    - Launch preemptive background worker task(s)\n");
    shell_print("    kill <pid>   - Terminate a background task\n");

    shell_print_color("  Storage & FAT12/FAT16 Filesystem:\n", 0x0B);
    shell_print("    ls (dir), cat <f>, edit <f>, write <f> <txt>, append <f> <txt>\n");
    shell_print("    cp <src> <dst>, mv <old> <new>, rm <f>, touch <f>, stat <f>\n");
    shell_print("    hexdump <f>, sh <script.sh>, df, disk, mkfs [label]\n");

    shell_print_color("  PCI & TCP/IP Networking:\n", 0x0B);
    shell_print("    lspci        - List all PCI bus devices\n");
    shell_print("    ifconfig     - View NIC/IP or set 'ifconfig <ip> <mask> <gw>'\n");
    shell_print("    dhcp         - Acquire IPv4 lease via DHCPv4 (DORA)\n");
    shell_print("    ping <ip>    - Send real ICMP Echo Requests & measure RTT\n");
    shell_print("    arp, netstat - View ARP cache or protocol packet counters\n\n");
}

static void cmd_info(void) {
    shell_print("\n");
    shell_print_color("      /\\_/\\      ", 0x0E);
    shell_print_color(username, 0x0E); shell_print_color("@bengal\n", 0x0A);

    shell_print_color("     ( o.o )     ", 0x0E);
    shell_print("-------------------------\n");

    shell_print_color("      > ^ <      ", 0x0E);
    shell_print_color("OS:      ", 0x0B);
    shell_print(OS_NAME " v" OS_VERSION " (" OS_CODENAME ")\n");

    shell_print_color("     /|   |\\     ", 0x0E);
    shell_print_color("Kernel:  ", 0x0B);
    shell_print("32-bit x86 Preemptive Multitasking\n");

    shell_print_color("    (_|   |_)    ", 0x0E);
    shell_print_color("CPU:     ", 0x0B);
    shell_print(cpu_get_brand()[0] ? cpu_get_brand() : cpu_get_vendor());
    shell_print("\n");

    shell_print("                 ");
    shell_print_color("Memory:  ", 0x0B);
    shell_print_int((int)(pmm_get_free_frames() * 4));
    shell_print(" KB free / ");
    shell_print_int((int)(pmm_get_total_frames() * 4));
    shell_print(" KB total\n");

    shell_print("                 ");
    shell_print_color("Display: ", 0x0B);
    if (vbe_is_active()) {
        shell_print_int((int)vbe_fb.width); shell_print("x");
        shell_print_int((int)vbe_fb.height); shell_print("x");
        shell_print_int((int)vbe_fb.bpp); shell_print(" VBE Framebuffer\n");
    } else {
        shell_print("VGA 80x25 Text Mode\n");
    }

    fat_fs_info_t fi;
    fat_get_fs_info(&fi);
    shell_print("                 ");
    shell_print_color("Storage: ", 0x0B);
    if (fi.mounted) {
        shell_print("FAT"); shell_print_int(fi.fat_type);
        shell_print(fi.is_ramdisk ? " (Live RAMDisk, " : " (ATA Disk, ");
        shell_print_int((int)(fi.free_bytes / 1024));
        shell_print(" KB free)\n");
    } else {
        shell_print("Unmounted\n");
    }

    shell_print("                 ");
    shell_print_color("Network: ", 0x0B);
    shell_print(nic_state.driver_name);
    if (net_config.configured) {
        char ip_s[20];
        net_format_ip(net_config.ip, ip_s);
        shell_print(" ("); shell_print(ip_s); shell_print(")");
    }
    shell_print("\n\n");
}

static void cmd_cpu(void) {
    shell_print_color("\nCPU Architecture & Features:\n", 0x0E);
    shell_print("  Vendor:   "); shell_print(cpu_get_vendor()); shell_print("\n");
    if (cpu_get_brand()[0]) {
        shell_print("  Brand:    "); shell_print(cpu_get_brand()); shell_print("\n");
    }
    shell_print("  Family:   "); shell_print_int(cpu_info.family);
    shell_print("  Model: "); shell_print_int(cpu_info.model);
    shell_print("  Stepping: "); shell_print_int(cpu_info.stepping); shell_print("\n");
    shell_print("  Features: ");
    if (cpu_info.has_fpu)  shell_print("FPU ");
    if (cpu_info.has_sse)  shell_print("SSE ");
    if (cpu_info.has_sse2) shell_print("SSE2 ");
    if (cpu_info.has_msr)  shell_print("MSR ");
    if (cpu_info.has_apic) shell_print("APIC ");
    shell_print("\n\n");
}

static void cmd_date(void) {
    rtc_time_t rt;
    rtc_read_time(&rt);
    char dt[24];
    rtc_format_datetime(&rt, dt);
    shell_print(dt);
    shell_print(" UTC\n");
}

static void cmd_uptime(void) {
    uint32_t s = timer_get_seconds();
    uint32_t d = s / 86400; s %= 86400;
    uint32_t h = s / 3600;  s %= 3600;
    uint32_t m = s / 60;    s %= 60;

    shell_print("Uptime: ");
    if (d > 0) { shell_print_int((int)d); shell_print("d "); }
    shell_print_int((int)h); shell_print("h ");
    shell_print_int((int)m); shell_print("m ");
    shell_print_int((int)s); shell_print("s (");
    char tbuf[16];
    k_utoa(timer_get_ticks(), tbuf, 10);
    shell_print(tbuf);
    shell_print(" ticks)\n");
}

static void cmd_mem(void) {
    heap_stats_t hs;
    heap_get_stats(&hs);

    shell_print_color("\nPhysical Memory Manager (PMM):\n", 0x0E);
    shell_print("  Total RAM:   "); shell_print_int((int)(pmm_get_total_frames() * 4)); shell_print(" KB\n");
    shell_print("  Used RAM:    "); shell_print_int((int)(pmm_get_used_frames() * 4));  shell_print(" KB\n");
    shell_print("  Free RAM:    "); shell_print_int((int)(pmm_get_free_frames() * 4));  shell_print(" KB\n");

    shell_print_color("\nDynamic Kernel Heap:\n", 0x0E);
    shell_print("  Heap Arena:  "); shell_print_int((int)(hs.total_size / 1024));       shell_print(" KB\n");
    shell_print("  Allocated:   "); shell_print_int((int)(hs.used_size));               shell_print(" B\n");
    shell_print("  Free Pool:   "); shell_print_int((int)(hs.free_size / 1024));        shell_print(" KB\n");
    shell_print("  Blocks:      "); shell_print_int((int)hs.num_allocations);
    shell_print(" active / "); shell_print_int((int)hs.num_blocks);
    shell_print(" total\n\n");
}

static void cmd_ps(void) {
    static const char *st_names[] = { "READY", "RUN  ", "WAIT ", "BLOCK", "ZOMB " };
    shell_print_color("\nPID  State  CPU Ticks     Name\n", 0x0E);
    shell_print("---- -----  ------------  ------------------------\n");

    int count = 0;
    task_t *head = scheduler_get_task_list(&count);
    task_t *curr = head;
    for (int i = 0; i < count && curr; i++) {
        shell_print_int((int)curr->id);
        if (curr->id < 10) shell_print("    ");
        else if (curr->id < 100) shell_print("   ");
        else shell_print("  ");

        uint32_t st = curr->state;
        shell_print_color((st <= 4) ? st_names[st] : "UNK  ",
                          (st == TASK_STATE_RUNNING) ? 0x0A : 0x07);
        shell_print("  ");

        char cbuf[16];
        k_utoa(curr->cpu_ticks, cbuf, 10);
        shell_print(cbuf);
        for (int k = strlen(cbuf); k < 14; k++) shell_putchar(' ');

        shell_print(curr->name);
        shell_putchar('\n');
        curr = curr->next;
    }
    shell_putchar('\n');
}

static void cmd_top(void) {
    shell_clear();
    while (1) {
        console_set_cursor(0, 0, 0);
        shell_print_color("Bengal Tiger OS - Live System Monitor (Press 'q' or ESC to quit)\n", 0x0E);
        cmd_uptime();
        cmd_mem();
        cmd_ps();

        for (int r = cur_y; r < con_rows - 1 && r < cur_y + 4; r++) {
            for (int c = 0; c < con_cols; c++) console_put_cell(c, r, ' ', default_attr);
        }

        uint32_t start = timer_get_ticks();
        while ((timer_get_ticks() - start) < 50) {
            int ev = input_poll_event();
            if (ev == 'q' || ev == 'Q' || ev == 27 || ev == CTRL_KEY('c')) {
                shell_clear();
                return;
            }
            nic_poll();
            if (irq_are_enabled()) hlt();
        }
    }
}

static void cmd_lspci(void) {
    uint32_t count = pci_get_device_count();

    shell_print_color("\nBus:Slot.Fn  Vendor:Device  IRQ  Vendor / Class\n", 0x0E);
    shell_print("-----------  -------------  ---  --------------------------------\n");
    for (uint32_t i = 0; i < count; i++) {
        pci_device_t *d = pci_get_device(i);
        if (!d) continue;
        print_hex_pad(d->bus, 2); shell_putchar(':');
        print_hex_pad(d->device, 2); shell_putchar('.');
        shell_print_int(d->function); shell_print("      ");
        print_hex_pad(d->vendor_id, 4); shell_putchar(':');
        print_hex_pad(d->device_id, 4); shell_print("      ");
        shell_print_int(d->irq);
        if (d->irq < 10) shell_print("    "); else shell_print("   ");
        shell_print(pci_vendor_name(d->vendor_id));
        shell_print(" - ");
        shell_print(pci_class_name(d->class_code));
        shell_putchar('\n');
    }
    shell_putchar('\n');
}

static void cmd_ifconfig(const char *args) {
    if (args && args[0]) {
        char t1[24] = "", t2[24] = "", t3[24] = "";
        int p = 0, k = 0;
        while (args[p] == ' ') p++;
        while (args[p] && args[p] != ' ' && k < 23) t1[k++] = args[p++];
        t1[k] = '\0'; k = 0;
        while (args[p] == ' ') p++;
        while (args[p] && args[p] != ' ' && k < 23) t2[k++] = args[p++];
        t2[k] = '\0'; k = 0;
        while (args[p] == ' ') p++;
        while (args[p] && args[p] != ' ' && k < 23) t3[k++] = args[p++];
        t3[k] = '\0';

        uint32_t ip = 0, mask = net_make_ip(255, 255, 255, 0), gw = 0;
        if (net_parse_ip(t1, &ip) != 0) {
            shell_print_color("Usage: ifconfig [<ip> [netmask] [gateway]]\n", 0x0C);
            return;
        }
        if (t2[0] && net_parse_ip(t2, &mask) != 0) {
            shell_print_color("Invalid subnet mask.\n", 0x0C);
            return;
        }
        if (t3[0] && net_parse_ip(t3, &gw) != 0) {
            shell_print_color("Invalid gateway IP.\n", 0x0C);
            return;
        }
        if (!gw) gw = (ip & mask) | 2U;
        net_set_static_ip(ip, mask, gw, net_make_ip(8, 8, 8, 8));
        console_refresh_topbar();
        shell_print_color("Static IPv4 configuration applied.\n", 0x0A);
    }

    shell_print_color("\neth0: ", 0x0E);
    shell_print(nic_state.driver_name);
    if (!nic_state.initialized) {
        shell_print(" (not present)\n\n");
        return;
    }

    shell_print(nic_state.link_up ? " <UP,BROADCAST,RUNNING>\n" : " <DOWN>\n");
    shell_print("      MAC:     ");
    for (int i = 0; i < 6; i++) {
        print_hex_pad(nic_state.mac[i], 2);
        if (i < 5) shell_putchar(':');
    }
    shell_print("  IRQ: "); shell_print_int(nic_state.irq); shell_putchar('\n');

    char s_ip[20], s_mask[20], s_gw[20], s_dns[20];
    net_format_ip(net_config.ip, s_ip);
    net_format_ip(net_config.netmask, s_mask);
    net_format_ip(net_config.gateway, s_gw);
    net_format_ip(net_config.dns, s_dns);

    shell_print("      inet:    "); shell_print(s_ip);
    shell_print("  netmask: "); shell_print(s_mask);
    shell_print(net_config.dhcp_leased ? "  (DHCP)\n" : "  (static)\n");
    shell_print("      gateway: "); shell_print(s_gw);
    shell_print("  dns:     "); shell_print(s_dns); shell_putchar('\n');
    shell_print("      RX pkts: "); shell_print_int((int)nic_state.rx_packets);
    shell_print(" ("); shell_print_int((int)nic_state.rx_bytes); shell_print(" B)  ");
    shell_print("TX pkts: "); shell_print_int((int)nic_state.tx_packets);
    shell_print(" ("); shell_print_int((int)nic_state.tx_bytes); shell_print(" B)\n\n");
}

static void cmd_dhcp(void) {
    if (!nic_state.initialized) {
        shell_print_color("No Ethernet controller detected.\n", 0x0C);
        return;
    }
    shell_print("Sending DHCPDISCOVER on eth0...\n");
    if (net_dhcp_discover(2500) == 0) {
        char s_ip[20], s_gw[20], s_dns[20];
        net_format_ip(net_config.ip, s_ip);
        net_format_ip(net_config.gateway, s_gw);
        net_format_ip(net_config.dns, s_dns);
        shell_print_color("DHCPACK bound to ", 0x0A);
        shell_print_color(s_ip, 0x0E);
        shell_print(" (gateway "); shell_print(s_gw);
        shell_print(", dns "); shell_print(s_dns); shell_print(")\n");
        console_refresh_topbar();
    } else {
        shell_print_color("DHCP request timed out (no DHCP server responded).\n", 0x0C);
    }
}

static void cmd_ping(const char *args) {
    if (!nic_state.initialized) {
        shell_print_color("Network interface unavailable.\n", 0x0C);
        return;
    }

    char ip_tok[32] = "";
    char cnt_tok[16] = "";
    int p = 0, k = 0;
    while (args[p] == ' ') p++;
    while (args[p] && args[p] != ' ' && k < 31) ip_tok[k++] = args[p++];
    ip_tok[k] = '\0'; k = 0;
    while (args[p] == ' ') p++;
    while (args[p] && args[p] != ' ' && k < 15) cnt_tok[k++] = args[p++];
    cnt_tok[k] = '\0';

    uint32_t dst_ip = 0;
    if (net_parse_ip(ip_tok, &dst_ip) != 0) {
        shell_print_color("Usage: ping <ipv4> [count]   (e.g. ping 10.0.2.2)\n", 0x0C);
        return;
    }

    int count = cnt_tok[0] ? k_atoi(cnt_tok) : 4;
    if (count < 1) count = 1;
    if (count > 20) count = 20;

    char ip_str[20];
    net_format_ip(dst_ip, ip_str);
    shell_print("PING "); shell_print(ip_str); shell_print(": 32 data bytes\n");

    int sent = 0, received = 0;
    uint32_t rtt_sum = 0;

    for (int seq = 1; seq <= count; seq++) {
        sent++;
        uint32_t rtt = 0;
        uint8_t ttl = 0;
        int rc = net_ping(dst_ip, (uint16_t)seq, 1200, &rtt, &ttl);
        if (rc == 0) {
            received++;
            rtt_sum += rtt;
            shell_print("40 bytes from "); shell_print(ip_str);
            shell_print(": icmp_seq="); shell_print_int(seq);
            shell_print(" ttl="); shell_print_int(ttl);
            shell_print(" time=");
            if (rtt == 0) shell_print("<1");
            else shell_print_int((int)rtt);
            shell_print(" ms\n");
        } else if (rc == -2) {
            shell_print_color("Destination Host Unreachable (ARP timeout)\n", 0x0C);
        } else {
            shell_print_color("Request timeout for icmp_seq ", 0x0C);
            shell_print_int(seq); shell_putchar('\n');
        }

        if (seq < count) {
            uint32_t wait_start = timer_get_ticks();
            while ((timer_get_ticks() - wait_start) < 25) {
                int ev = input_poll_event();
                if (ev == CTRL_KEY('c')) {
                    shell_print("^C\n");
                    goto stats;
                }
                nic_poll();
                if (irq_are_enabled()) hlt();
            }
        }
    }

stats:
    shell_print("--- "); shell_print(ip_str); shell_print(" ping statistics ---\n");
    shell_print_int(sent); shell_print(" packets transmitted, ");
    shell_print_int(received); shell_print(" packets received, ");
    shell_print_int(((sent - received) * 100) / (sent ? sent : 1));
    shell_print("% packet loss");
    if (received > 0) {
        shell_print(", avg rtt = ");
        shell_print_int((int)(rtt_sum / (uint32_t)received));
        shell_print(" ms");
    }
    shell_putchar('\n');
}

static void cmd_arp(void) {
    int max_e = 0;
    const arp_entry_t *tbl = net_get_arp_table(&max_e);
    shell_print_color("\nIPv4 Address      MAC Address\n", 0x0E);
    shell_print("----------------  -----------------\n");
    int shown = 0;
    for (int i = 0; i < max_e; i++) {
        if (!tbl[i].valid) continue;
        char ip_s[20];
        net_format_ip(tbl[i].ip, ip_s);
        shell_print(ip_s);
        for (int k = strlen(ip_s); k < 18; k++) shell_putchar(' ');
        for (int b = 0; b < 6; b++) {
            print_hex_pad(tbl[i].mac[b], 2);
            if (b < 5) shell_putchar(':');
        }
        shell_putchar('\n');
        shown++;
    }
    if (!shown) shell_print("(ARP cache empty)\n");
    shell_putchar('\n');
}

static void cmd_netstat(void) {
    shell_print_color("\nNetwork Protocol Statistics:\n", 0x0E);
    shell_print("  Ethernet RX: "); shell_print_int((int)net_config.rx_eth);  shell_putchar('\n');
    shell_print("  ARP RX:      "); shell_print_int((int)net_config.rx_arp);  shell_putchar('\n');
    shell_print("  IPv4 RX:     "); shell_print_int((int)net_config.rx_ipv4); shell_putchar('\n');
    shell_print("  ICMP RX:     "); shell_print_int((int)net_config.rx_icmp); shell_putchar('\n');
    shell_print("  UDP RX:      "); shell_print_int((int)net_config.rx_udp);  shell_putchar('\n');
    shell_print("  NIC TX Pkts: "); shell_print_int((int)nic_state.tx_packets); shell_print("\n\n");
}

static void cmd_disk(void) {
    shell_print_color("\nStorage Controllers & Drives:\n", 0x0E);
    int found = 0;
    for (int i = 0; i < 4; i++) {
        ata_drive_t *d = ata_get_drive(i);
        if (!d || !d->present) continue;
        found++;
        shell_print("  Slot "); shell_print_int(i); shell_print(": ");
        shell_print(d->drive_type == ATA_TYPE_ATAPI ? "[ATAPI CD/DVD] " : "[ATA HDD]      ");
        shell_print(d->model);
        uint32_t secs = d->sectors_28 ? d->sectors_28 : (uint32_t)d->sectors_48;
        if (secs > 0) {
            shell_print(" (");
            shell_print_int((int)(secs / 2));
            shell_print(" KB)");
        }
        shell_putchar('\n');
    }
    if (!found) {
        shell_print("  No physical ATA/ATAPI drives attached.\n");
    }
    shell_print("  Active FS Backend: ");
    shell_print_color(disk_is_ramdisk() ? "512KB Live RAMDisk" : "Persistent ATA Hard Disk", 0x0A);
    shell_print("\n\n");
}

static void cmd_df(void) {
    fat_fs_info_t fi;
    fat_get_fs_info(&fi);
    if (!fi.mounted) {
        shell_print_color("No filesystem mounted. Run 'mkfs' to format.\n", 0x0C);
        return;
    }
    shell_print_color("\nFilesystem   Type   Total KB  Used KB   Free KB   Files  Backend\n", 0x0E);
    shell_print("-----------  -----  --------  --------  --------  -----  ----------------\n");
    const char *lbl = fi.label[0] ? fi.label : "BENGAL";
    shell_print(lbl);
    for (int k = strlen(lbl); k < 13; k++) shell_putchar(' ');
    shell_print("FAT"); shell_print_int(fi.fat_type); shell_print("  ");

    char nbuf[16];
    k_utoa(fi.total_bytes / 1024, nbuf, 10); shell_print(nbuf);
    for (int k = strlen(nbuf); k < 10; k++) shell_putchar(' ');
    k_utoa(fi.used_bytes / 1024, nbuf, 10);  shell_print(nbuf);
    for (int k = strlen(nbuf); k < 10; k++) shell_putchar(' ');
    k_utoa(fi.free_bytes / 1024, nbuf, 10);  shell_print(nbuf);
    for (int k = strlen(nbuf); k < 10; k++) shell_putchar(' ');
    k_utoa(fi.file_count, nbuf, 10);         shell_print(nbuf);
    for (int k = strlen(nbuf); k < 7; k++)  shell_putchar(' ');
    shell_print(fi.is_ramdisk ? "Live RAMDisk\n\n" : "Persistent ATA\n\n");
}

static void cmd_hexdump(const char *fname) {
    fat_file_t st;
    if (fat_stat_file(fname, &st) != 0) {
        shell_print_color("File not found: ", 0x0C); shell_print(fname); shell_putchar('\n');
        return;
    }
    uint32_t cap = (st.size > 512) ? 512 : st.size;
    if (cap == 0) {
        shell_print("(empty file)\n");
        return;
    }
    uint8_t *buf = (uint8_t*)kmalloc(cap);
    if (!buf) return;
    if (fat_read_file(fname, buf, cap, NULL) == 0) {
        for (uint32_t off = 0; off < cap; off += 16) {
            print_hex_pad(off, 4);
            shell_print("  ");
            for (uint32_t j = 0; j < 16; j++) {
                if (off + j < cap) {
                    print_hex_pad(buf[off + j], 2);
                    shell_putchar(' ');
                } else {
                    shell_print("   ");
                }
            }
            shell_print(" |");
            for (uint32_t j = 0; j < 16 && (off + j) < cap; j++) {
                uint8_t c = buf[off + j];
                shell_putchar((c >= 32 && c <= 126) ? (char)c : '.');
            }
            shell_print("|\n");
        }
        if (st.size > cap) {
            shell_print("... (truncated at 512 bytes; total ");
            shell_print_int((int)st.size);
            shell_print(" bytes)\n");
        }
    }
    kfree(buf);
}

static void cmd_sh(const char *fname) {
    if (script_depth >= 4) {
        shell_print_color("Maximum script nesting depth (4) exceeded.\n", 0x0C);
        return;
    }
    fat_file_t st;
    if (fat_stat_file(fname, &st) != 0) {
        shell_print_color("Script not found: ", 0x0C); shell_print(fname); shell_putchar('\n');
        return;
    }
    if (st.size == 0 || st.size > 16384) return;

    char *script = (char*)kmalloc(st.size + 1);
    if (!script) return;
    if (fat_read_file(fname, script, st.size, NULL) == 0) {
        script[st.size] = '\0';
        script_depth++;
        char line[CMD_BUF_MAX];
        uint32_t p = 0;
        while (p < st.size) {
            int k = 0;
            while (p < st.size && script[p] != '\n' && script[p] != '\r') {
                if (k < CMD_BUF_MAX - 1) line[k++] = script[p];
                p++;
            }
            line[k] = '\0';
            while (p < st.size && (script[p] == '\n' || script[p] == '\r')) p++;

            const char *trim = line;
            while (*trim == ' ' || *trim == '\t') trim++;
            if (*trim == '\0' || *trim == '#') continue;
            shell_execute_line(trim);
        }
        script_depth--;
    }
    kfree(script);
}

static void split_two_args(const char *args, char *a1, int max1, char *a2, int max2) {
    int p = 0, k = 0;
    while (args[p] == ' ') p++;
    while (args[p] && args[p] != ' ' && k < max1 - 1) a1[k++] = args[p++];
    a1[k] = '\0';
    while (args[p] == ' ') p++;
    k = 0;
    while (args[p] && k < max2 - 1) a2[k++] = args[p++];
    a2[k] = '\0';
}

/* =========================================================================
 * Command Parser & Executor
 * ========================================================================= */
void shell_execute_line(const char *raw_line) {
    if (!raw_line) return;

    const char *p = raw_line;
    while (*p == ' ' || *p == '\t') p++;
    if (*p == '\0' || *p == '#') return;

    if (p[0] == '!' && p[1] >= '1' && p[1] <= '9') {
        int idx = k_atoi(p + 1) - 1;
        if (idx >= 0 && idx < history_count) {
            shell_print_color(history[idx], 0x08);
            shell_putchar('\n');
            p = history[idx];
        } else {
            shell_print_color("Event not found in history.\n", 0x0C);
            return;
        }
    }

    char verb[32];
    int vk = 0;
    while (*p && *p != ' ' && *p != '\t' && vk < (int)sizeof(verb) - 1) {
        char c = *p++;
        if (c >= 'A' && c <= 'Z') c = (char)(c - 'A' + 'a');
        verb[vk++] = c;
    }
    verb[vk] = '\0';
    while (*p == ' ' || *p == '\t') p++;
    const char *args = p;

    if (strcmp(verb, "help") == 0) {
        cmd_help();
    } else if (strcmp(verb, "info") == 0 || strcmp(verb, "neofetch") == 0) {
        cmd_info();
    } else if (strcmp(verb, "ver") == 0 || strcmp(verb, "version") == 0) {
        shell_print(OS_NAME " v" OS_VERSION " (" OS_CODENAME ") - Built for 32-bit x86\n");
    } else if (strcmp(verb, "cpu") == 0) {
        cmd_cpu();
    } else if (strcmp(verb, "date") == 0) {
        cmd_date();
    } else if (strcmp(verb, "uptime") == 0) {
        cmd_uptime();
    } else if (strcmp(verb, "dmesg") == 0) {
        uint32_t len = 0;
        const char *buf = klog_get_buffer(&len);
        if (buf && len > 0) shell_print(buf);
        else shell_print("(kernel log empty)\n");
    } else if (strcmp(verb, "user") == 0) {
        if (!args[0]) {
            shell_print("Current user: "); shell_print_color(username, 0x0E); shell_putchar('\n');
        } else {
            strncpy(username, args, sizeof(username) - 1);
            username[sizeof(username) - 1] = '\0';
            for (int i = 0; username[i]; i++) {
                if (username[i] == ' ') { username[i] = '\0'; break; }
            }
            fat_write_file("CONFIG.CFG", username, (uint32_t)strlen(username));
            shell_print("Active user updated to '"); shell_print_color(username, 0x0E); shell_print("'.\n");
        }
    } else if (strcmp(verb, "history") == 0) {
        for (int i = 0; i < history_count; i++) {
            shell_print("  "); shell_print_int(i + 1); shell_print("  ");
            shell_print(history[i]); shell_putchar('\n');
        }
    } else if (strcmp(verb, "echo") == 0) {
        const char *redir = strstr(args, ">>");
        int append_mode = 1;
        if (!redir) {
            redir = strchr(args, '>');
            append_mode = 0;
        }
        if (redir) {
            int text_len = (int)(redir - args);
            while (text_len > 0 && args[text_len - 1] == ' ') text_len--;
            const char *fn = redir + (append_mode ? 2 : 1);
            while (*fn == ' ') fn++;
            if (!*fn) {
                shell_print_color("Missing redirection filename.\n", 0x0C);
                return;
            }
            char *tmp = (char*)kmalloc((uint32_t)text_len + 2);
            if (tmp) {
                memcpy(tmp, args, (uint32_t)text_len);
                tmp[text_len]     = '\n';
                tmp[text_len + 1] = '\0';
                int rc = append_mode
                    ? fat_append_file(fn, tmp, (uint32_t)text_len + 1)
                    : fat_write_file(fn, tmp, (uint32_t)text_len + 1);
                kfree(tmp);
                if (rc != 0) shell_print_color("Failed to write redirected output.\n", 0x0C);
            }
        } else {
            shell_print(args);
            shell_putchar('\n');
        }
    } else if (strcmp(verb, "calc") == 0) {
        if (!args[0]) {
            shell_print("Usage: calc <expression>  (e.g. calc (12 + 8) * 5)\n");
            return;
        }
        calc_ctx_t ctx = { args, 0, 0 };
        int res = calc_parse_expr(&ctx);
        calc_skip_ws(&ctx);
        if (ctx.s[ctx.pos] != '\0' && !ctx.err) ctx.err = 1;
        if (ctx.err == 2) {
            shell_print_color("Error: Division by zero.\n", 0x0C);
        } else if (ctx.err) {
            shell_print_color("Error: Invalid expression syntax.\n", 0x0C);
        } else {
            shell_print("= ");
            shell_print_int(res);
            shell_print(" ("); shell_print_hex((uint32_t)res); shell_print(")\n");
        }
    } else if (strcmp(verb, "beep") == 0) {
        char f_s[16], d_s[16];
        split_two_args(args, f_s, 16, d_s, 16);
        int freq = f_s[0] ? k_atoi(f_s) : 880;
        int dur  = d_s[0] ? k_atoi(d_s) : 150;
        if (dur > 3000) dur = 3000;
        speaker_beep((uint32_t)freq, (uint32_t)dur);
    } else if (strcmp(verb, "gui") == 0) {
        cmd_gui();
    } else if (strcmp(verb, "clear") == 0 || strcmp(verb, "cls") == 0) {
        shell_clear();
    } else if (strcmp(verb, "mem") == 0 || strcmp(verb, "free") == 0) {
        cmd_mem();
    } else if (strcmp(verb, "ps") == 0) {
        cmd_ps();
    } else if (strcmp(verb, "top") == 0) {
        cmd_top();
    } else if (strcmp(verb, "spawn") == 0) {
        int n = args[0] ? k_atoi(args) : 1;
        if (n < 1) n = 1;
        if (n > 8) n = 8;
        for (int i = 0; i < n; i++) {
            task_t *t = task_create("tiger_worker", bg_worker_entry);
            if (t) {
                shell_print_color("Spawned background task PID ", 0x0A);
                shell_print_int((int)t->id); shell_putchar('\n');
            } else {
                shell_print_color("Task table full.\n", 0x0C);
                break;
            }
        }
    } else if (strcmp(verb, "kill") == 0) {
        if (!args[0]) { shell_print("Usage: kill <pid>\n"); return; }
        uint32_t pid = (uint32_t)k_atoi(args);
        if (pid == 0) {
            shell_print_color("Cannot kill PID 0 (kernel shell task).\n", 0x0C);
        } else if (task_kill(pid) == 0) {
            shell_print("Terminated task PID "); shell_print_int((int)pid); shell_putchar('\n');
        } else {
            shell_print_color("No active task with that PID.\n", 0x0C);
        }
    } else if (strcmp(verb, "ls") == 0 || strcmp(verb, "dir") == 0) {
        fat_list_files();
    } else if (strcmp(verb, "cat") == 0) {
        if (!args[0]) { shell_print("Usage: cat <filename>\n"); return; }
        fat_file_t st;
        if (fat_stat_file(args, &st) != 0) {
            shell_print_color("File not found: ", 0x0C); shell_print(args); shell_putchar('\n');
            return;
        }
        if (st.size == 0) return;
        uint32_t cap = (st.size > FAT_MAX_FILE_SIZE) ? FAT_MAX_FILE_SIZE : st.size;
        char *buf = (char*)kmalloc(cap + 1);
        if (buf) {
            if (fat_read_file(args, buf, cap, NULL) == 0) {
                buf[cap] = '\0';
                shell_print(buf);
                if (buf[cap - 1] != '\n') shell_putchar('\n');
            }
            kfree(buf);
        }
    } else if (strcmp(verb, "write") == 0) {
        char fn[32], content[CMD_BUF_MAX];
        split_two_args(args, fn, sizeof(fn), content, sizeof(content));
        if (!fn[0]) { shell_print("Usage: write <filename> <text>\n"); return; }
        int len = strlen(content);
        if (len + 2 < (int)sizeof(content)) {
            content[len] = '\n';
            content[len + 1] = '\0';
            len++;
        }
        if (fat_write_file(fn, content, (uint32_t)len) == 0) {
            shell_print_color("Saved ", 0x0A); shell_print_int(len);
            shell_print(" bytes to "); shell_print(fn); shell_putchar('\n');
        } else {
            shell_print_color("Failed to write file.\n", 0x0C);
        }
    } else if (strcmp(verb, "append") == 0) {
        char fn[32], content[CMD_BUF_MAX];
        split_two_args(args, fn, sizeof(fn), content, sizeof(content));
        if (!fn[0]) { shell_print("Usage: append <filename> <text>\n"); return; }
        int len = strlen(content);
        if (len + 2 < (int)sizeof(content)) {
            content[len] = '\n';
            content[len + 1] = '\0';
            len++;
        }
        if (fat_append_file(fn, content, (uint32_t)len) == 0) {
            shell_print_color("Appended ", 0x0A); shell_print_int(len);
            shell_print(" bytes to "); shell_print(fn); shell_putchar('\n');
        } else {
            shell_print_color("Failed to append file.\n", 0x0C);
        }
    } else if (strcmp(verb, "touch") == 0) {
        if (!args[0]) { shell_print("Usage: touch <filename>\n"); return; }
        if (!fat_file_exists(args)) {
            if (fat_write_file(args, "", 0) != 0) {
                shell_print_color("Failed to create file.\n", 0x0C);
            }
        }
    } else if (strcmp(verb, "rm") == 0 || strcmp(verb, "del") == 0) {
        if (!args[0]) { shell_print("Usage: rm <filename>\n"); return; }
        if (fat_delete_file(args) == 0) {
            shell_print("Deleted "); shell_print(args); shell_putchar('\n');
        } else {
            shell_print_color("File not found: ", 0x0C); shell_print(args); shell_putchar('\n');
        }
    } else if (strcmp(verb, "cp") == 0) {
        char src[32], dst[32];
        split_two_args(args, src, 32, dst, 32);
        if (!src[0] || !dst[0]) { shell_print("Usage: cp <src> <dst>\n"); return; }
        if (fat_copy_file(src, dst) == 0) {
            shell_print("Copied "); shell_print(src); shell_print(" -> "); shell_print(dst); shell_putchar('\n');
        } else {
            shell_print_color("Copy failed.\n", 0x0C);
        }
    } else if (strcmp(verb, "mv") == 0) {
        char src[32], dst[32];
        split_two_args(args, src, 32, dst, 32);
        if (!src[0] || !dst[0]) { shell_print("Usage: mv <old> <new>\n"); return; }
        if (fat_rename_file(src, dst) == 0) {
            shell_print("Renamed "); shell_print(src); shell_print(" -> "); shell_print(dst); shell_putchar('\n');
        } else {
            shell_print_color("Rename failed (check source exists and target is free).\n", 0x0C);
        }
    } else if (strcmp(verb, "stat") == 0) {
        if (!args[0]) { shell_print("Usage: stat <filename>\n"); return; }
        fat_file_t st;
        if (fat_stat_file(args, &st) == 0) {
            shell_print("  File:    "); shell_print_color(st.name, 0x0E); shell_putchar('\n');
            shell_print("  Size:    "); shell_print_int((int)st.size); shell_print(" bytes\n");
            shell_print("  Cluster: "); shell_print_int((int)st.first_cluster); shell_putchar('\n');
            shell_print("  Attr:    "); shell_print_hex(st.attr); shell_putchar('\n');
        } else {
            shell_print_color("File not found.\n", 0x0C);
        }
    } else if (strcmp(verb, "hexdump") == 0) {
        if (!args[0]) { shell_print("Usage: hexdump <filename>\n"); return; }
        cmd_hexdump(args);
    } else if (strcmp(verb, "edit") == 0) {
        if (!args[0]) { shell_print("Usage: edit <filename>\n"); return; }
        editor_open(args);
    } else if (strcmp(verb, "sh") == 0) {
        if (!args[0]) { shell_print("Usage: sh <script.sh>\n"); return; }
        cmd_sh(args);
    } else if (strcmp(verb, "df") == 0) {
        cmd_df();
    } else if (strcmp(verb, "disk") == 0) {
        cmd_disk();
    } else if (strcmp(verb, "mkfs") == 0) {
        int use_ata = disk_has_hardware_hdd() ? 1 : 0;
        const char *lbl = args[0] ? args : (use_ata ? "BENGALHDD" : "BENGALLIVE");
        shell_print(use_ata ? "Formatting hardware ATA disk as FAT...\n"
                            : "Formatting 512KB Live RAMDisk as FAT12...\n");
        if (fat_format(lbl, use_ata) == 0) {
            console_refresh_topbar();
            shell_print_color("Filesystem formatted and mounted successfully.\n", 0x0A);
            cmd_df();
        } else {
            shell_print_color("mkfs failed.\n", 0x0C);
        }
    } else if (strcmp(verb, "lspci") == 0) {
        cmd_lspci();
    } else if (strcmp(verb, "ifconfig") == 0 || strcmp(verb, "ip") == 0) {
        cmd_ifconfig(args);
    } else if (strcmp(verb, "dhcp") == 0) {
        cmd_dhcp();
    } else if (strcmp(verb, "ping") == 0) {
        cmd_ping(args);
    } else if (strcmp(verb, "arp") == 0) {
        cmd_arp();
    } else if (strcmp(verb, "netstat") == 0) {
        cmd_netstat();
    } else if (strcmp(verb, "reboot") == 0) {
        shell_print_color("Rebooting Bengal Tiger OS...\n", 0x0E);
        cli();
        int timeout = 10000;
        while (timeout-- > 0 && (inb(0x64) & 0x02));
        outb(0x64, 0xFE);
        struct { uint16_t lim; uint32_t base; } __attribute__((packed)) null_idt = { 0, 0 };
        __asm__ volatile("lidt %0; int3" : : "m"(null_idt));
        while (1) hlt();
    } else if (strcmp(verb, "shutdown") == 0 || strcmp(verb, "poweroff") == 0) {
        shell_print_color("Shutting down Bengal Tiger OS...\n", 0x0E);
        outw(0x604,  0x2000); /* QEMU / KVM */
        outw(0x4004, 0x3400); /* VirtualBox */
        outw(0xB004, 0x2000); /* Bochs / older QEMU */
        shell_print("It is now safe to power off your computer.\n");
        cli();
        while (1) hlt();
    } else {
        shell_print_color("Unknown command: '", 0x0C);
        shell_print_color(verb, 0x0C);
        shell_print_color("'. Type 'help' for available commands.\n", 0x07);
    }
}

/* =========================================================================
 * Shell Input Event Processing & Main Loop
 * ========================================================================= */
void shell_handle_event(int ev) {
    if (ev <= 0) return;

    int prev_len = cmd_len;

    if (ev == '\n') {
        cmd_cursor = cmd_len;
        redraw_input_line(prev_len);
        shell_putchar('\n');

        cmd_buf[cmd_len] = '\0';
        char line_copy[CMD_BUF_MAX];
        strcpy(line_copy, cmd_buf);

        cmd_len     = 0;
        cmd_cursor  = 0;
        cmd_buf[0]  = '\0';
        history_nav = -1;

        history_push(line_copy);
        shell_execute_line(line_copy);
        shell_print_prompt();
    } else if (ev == CTRL_KEY('c')) {
        cmd_cursor = cmd_len;
        redraw_input_line(prev_len);
        shell_print("^C\n");
        cmd_len     = 0;
        cmd_cursor  = 0;
        cmd_buf[0]  = '\0';
        history_nav = -1;
        shell_print_prompt();
    } else if (ev == CTRL_KEY('l')) {
        shell_clear();
        shell_print_prompt();
        redraw_input_line(0);
    } else if (ev == '\b') {
        if (cmd_cursor > 0) {
            memmove(cmd_buf + cmd_cursor - 1, cmd_buf + cmd_cursor, (uint32_t)(cmd_len - cmd_cursor + 1));
            cmd_cursor--;
            cmd_len--;
            redraw_input_line(prev_len);
        }
    } else if (ev == KEY_SPECIAL_DELETE) {
        if (cmd_cursor < cmd_len) {
            memmove(cmd_buf + cmd_cursor, cmd_buf + cmd_cursor + 1, (uint32_t)(cmd_len - cmd_cursor));
            cmd_len--;
            redraw_input_line(prev_len);
        }
    } else if (ev == KEY_SPECIAL_LEFT) {
        if (cmd_cursor > 0) {
            cmd_cursor--;
            redraw_input_line(prev_len);
        }
    } else if (ev == KEY_SPECIAL_RIGHT) {
        if (cmd_cursor < cmd_len) {
            cmd_cursor++;
            redraw_input_line(prev_len);
        }
    } else if (ev == KEY_SPECIAL_HOME || ev == CTRL_KEY('a')) {
        cmd_cursor = 0;
        redraw_input_line(prev_len);
    } else if (ev == KEY_SPECIAL_END || ev == CTRL_KEY('e')) {
        cmd_cursor = cmd_len;
        redraw_input_line(prev_len);
    } else if (ev == KEY_SPECIAL_UP) {
        if (history_count > 0) {
            if (history_nav < 0) history_nav = history_count - 1;
            else if (history_nav > 0) history_nav--;
            strcpy(cmd_buf, history[history_nav]);
            cmd_len    = strlen(cmd_buf);
            cmd_cursor = cmd_len;
            redraw_input_line(prev_len);
        }
    } else if (ev == KEY_SPECIAL_DOWN) {
        if (history_nav >= 0) {
            history_nav++;
            if (history_nav >= history_count) {
                history_nav = -1;
                cmd_buf[0]  = '\0';
                cmd_len     = 0;
                cmd_cursor  = 0;
            } else {
                strcpy(cmd_buf, history[history_nav]);
                cmd_len    = strlen(cmd_buf);
                cmd_cursor = cmd_len;
            }
            redraw_input_line(prev_len);
        }
    } else if (ev == '\t') {
        handle_tab_completion();
    } else if (ev == KEY_SPECIAL_F1) {
        shell_putchar('\n');
        cmd_help();
        shell_print_prompt();
        redraw_input_line(0);
    } else if (ev == KEY_SPECIAL_F2) {
        shell_putchar('\n');
        cmd_gui();
        shell_print_prompt();
        redraw_input_line(0);
    } else if (ev >= 32 && ev <= 126) {
        if (cmd_len < CMD_BUF_MAX - 1) {
            memmove(cmd_buf + cmd_cursor + 1, cmd_buf + cmd_cursor, (uint32_t)(cmd_len - cmd_cursor + 1));
            cmd_buf[cmd_cursor] = (char)ev;
            cmd_cursor++;
            cmd_len++;
            redraw_input_line(prev_len);
        }
    }
}

void shell_handler(char key) {
    shell_handle_event((unsigned char)key);
}

void shell_init(void) {
    if (vbe_is_active()) {
        con_cols = (int)vbe_fb.width / FONT_WIDTH;
        con_rows = ((int)vbe_fb.height - VBE_TOPBAR_HEIGHT) / FONT_HEIGHT;
        if (con_cols > MAX_GRID_COLS) con_cols = MAX_GRID_COLS;
        if (con_rows > MAX_GRID_ROWS) con_rows = MAX_GRID_ROWS;
        if (con_cols < 40) con_cols = 40;
        if (con_rows < 15) con_rows = 15;
    } else {
        con_cols = 80;
        con_rows = 25;
    }

    cmd_len     = 0;
    cmd_cursor  = 0;
    cmd_buf[0]  = '\0';
    history_nav = -1;

    char cfg[32];
    memset(cfg, 0, sizeof(cfg));
    if (fat_read_file("CONFIG.CFG", cfg, sizeof(cfg) - 1, NULL) == 0 && cfg[0]) {
        int k = 0;
        while (cfg[k] && cfg[k] != '\n' && cfg[k] != '\r' && cfg[k] != ' ' && k < 30) {
            username[k] = cfg[k];
            k++;
        }
        username[k] = '\0';
        if (!username[0]) strcpy(username, "tiger");
    }

    shell_clear();

    shell_print_color("================================================================\n", 0x0E);
    shell_print_color("  Bengal Tiger OS v" OS_VERSION " (" OS_CODENAME ") - Ready\n", 0x0E);
    shell_print_color("================================================================\n", 0x0E);
    shell_print("  Type 'help' for commands, 'info' for system summary, 'edit' to edit files,\n");
    shell_print("  'gui' (or F2) for graphical desktop, 'ping 10.0.2.2' to test networking.\n\n");
}

void shell_run(void) {
    console_refresh_topbar();
    shell_print_prompt();

    uint32_t last_clock_tick = timer_get_ticks();

    while (1) {
        int ev;
        while ((ev = input_poll_event()) != 0) {
            shell_handle_event(ev);
        }

        nic_poll();

        uint32_t now = timer_get_ticks();
        if ((now - last_clock_tick) >= 100) {
            console_refresh_topbar();
            last_clock_tick = now;
        }

        if (irq_are_enabled()) {
            hlt();
        }
    }
}

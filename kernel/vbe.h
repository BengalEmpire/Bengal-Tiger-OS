/**
 * Bengal Tiger OS - VBE Linear Framebuffer Driver
 *
 * High-resolution 24/32-bit VESA framebuffer graphics engine with hardware
 * channel-layout adaptation, fast region scrolling, and 8x16 glyph rendering.
 *
 * @file vbe.h
 * @author Bengal Tiger OS (BengalEmpire)
 * @version 1.0.0
 */

#ifndef VBE_H
#define VBE_H

#include "common.h"

#define FB_TYPE_INDEXED    0
#define FB_TYPE_RGB        1
#define FB_TYPE_TEXT       2

/* Canonical 0x00RRGGBB color constants */
#define VBE_BLACK         0x00000000U
#define VBE_BLUE          0x000000AAU
#define VBE_GREEN         0x0000AA00U
#define VBE_CYAN          0x0000AAAAU
#define VBE_RED           0x00AA0000U
#define VBE_MAGENTA       0x00AA00AAU
#define VBE_BROWN         0x00AA5500U
#define VBE_LIGHT_GREY    0x00AAAAAAU
#define VBE_DARK_GREY     0x00555555U
#define VBE_LIGHT_BLUE    0x005555FFU
#define VBE_LIGHT_GREEN   0x0055FF55U
#define VBE_LIGHT_CYAN    0x0055FFFFU
#define VBE_LIGHT_RED     0x00FF5555U
#define VBE_LIGHT_MAGENTA 0x00FF55FFU
#define VBE_YELLOW        0x00FFFF55U
#define VBE_WHITE         0x00FFFFFFU

#define VBE_COLOR_BLACK         0
#define VBE_COLOR_BLUE          1
#define VBE_COLOR_GREEN         2
#define VBE_COLOR_CYAN          3
#define VBE_COLOR_RED           4
#define VBE_COLOR_MAGENTA       5
#define VBE_COLOR_BROWN         6
#define VBE_COLOR_LIGHT_GREY    7
#define VBE_COLOR_DARK_GREY     8
#define VBE_COLOR_LIGHT_BLUE    9
#define VBE_COLOR_LIGHT_GREEN   10
#define VBE_COLOR_LIGHT_CYAN    11
#define VBE_COLOR_LIGHT_RED     12
#define VBE_COLOR_LIGHT_MAGENTA 13
#define VBE_COLOR_YELLOW        14
#define VBE_COLOR_WHITE         15

#define VBE_DEFAULT_BG VBE_BLACK
#define VBE_DEFAULT_FG VBE_YELLOW

extern const uint32_t vbe_palette[16];

typedef struct {
    uint8_t  initialized;
    uint8_t  fb_type;
    uint32_t width;
    uint32_t height;
    uint32_t pitch;
    uint8_t  bpp;
    uint32_t phys_addr;
    uint32_t virt_addr;
    uint8_t  red_pos;
    uint8_t  red_size;
    uint8_t  green_pos;
    uint8_t  green_size;
    uint8_t  blue_pos;
    uint8_t  blue_size;
} vbe_fb_t;

extern vbe_fb_t vbe_fb;
extern uint32_t vbe_fg_color;
extern uint32_t vbe_bg_color;

int vbe_init(uint32_t mbi_flags,
             uint64_t fb_addr,
             uint32_t fb_pitch,
             uint32_t fb_width,
             uint32_t fb_height,
             uint8_t  fb_bpp,
             uint8_t  fb_type,
             uint8_t  red_pos, uint8_t red_size,
             uint8_t  green_pos, uint8_t green_size,
             uint8_t  blue_pos, uint8_t blue_size);

int      vbe_is_active(void);
void     vbe_putpixel(uint32_t x, uint32_t y, uint32_t color);
uint32_t vbe_getpixel(uint32_t x, uint32_t y);
void     vbe_fill_rect(uint32_t x, uint32_t y, uint32_t w, uint32_t h, uint32_t color);
void     vbe_draw_rect(uint32_t x, uint32_t y, uint32_t w, uint32_t h, uint32_t color, uint32_t thickness);
void     vbe_draw_hline(uint32_t x, uint32_t y, uint32_t len, uint32_t color);
void     vbe_draw_vline(uint32_t x, uint32_t y, uint32_t len, uint32_t color);
void     vbe_clear(void);
void     vbe_clear_color(uint32_t color);
void     vbe_scroll(uint32_t lines);
void     vbe_scroll_region(uint32_t top_y, uint32_t region_h, uint32_t lines, uint32_t bg_color);
void     vbe_draw_char(uint32_t x, uint32_t y, char c, uint32_t fg, uint32_t bg);
void     vbe_draw_string(uint32_t x, uint32_t y, const char *str, uint32_t fg, uint32_t bg);
void     vbe_puts(uint32_t x, uint32_t y, const char *str);
uint32_t vbe_vga_to_rgb(uint8_t vga_color);

static inline uint32_t vbe_rgb(uint8_t r, uint8_t g, uint8_t b) {
    return ((uint32_t)r << 16) | ((uint32_t)g << 8) | (uint32_t)b;
}

#endif /* VBE_H */

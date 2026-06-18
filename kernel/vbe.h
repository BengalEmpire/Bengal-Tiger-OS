/**
 * Bengal Tiger OS - VBE Framebuffer Driver
 *
 * Provides graphics mode support via VESA BIOS Extensions (VBE).
 * Uses the linear framebuffer set up by GRUB via gfxpayload.
 * Supports 24/32-bit color modes with standard RGB pixel format.
 *
 * Features:
 *   - Pixel, rectangle, line drawing
 *   - Text rendering with 8x16 bitmap font
 *   - Screen clear and scroll
 *   - Hardware cursor management
 *
 * @file vbe.h
 * @author Bengal Tiger OS (BengalEmpire)
 * @version 0.4.0
 */

#ifndef VBE_H
#define VBE_H

#include "common.h"

/* Framebuffer types (from multiboot spec) */
#define FB_TYPE_RGB        1   /* Standard RGB color */
#define FB_TYPE_TEXT       2   /* VGA text mode */
#define FB_TYPE_INDEXED    3   /* Indexed color */

/* VBE color (32-bit ARGB format) */
#define VBE_BLACK       0x00000000
#define VBE_BLUE        0x000000AA
#define VBE_GREEN       0x0000AA00
#define VBE_CYAN        0x0000AAAA
#define VBE_RED         0x00AA0000
#define VBE_MAGENTA     0x00AA00AA
#define VBE_BROWN       0x00AA5500
#define VBE_LIGHT_GREY  0x00AAAAAA
#define VBE_DARK_GREY   0x00555555
#define VBE_LIGHT_BLUE  0x005555FF
#define VBE_LIGHT_GREEN 0x0055FF55
#define VBE_LIGHT_CYAN  0x0055FFFF
#define VBE_LIGHT_RED   0x00FF5555
#define VBE_LIGHT_MAGENTA 0x00FF55FF
#define VBE_YELLOW      0x00FFFF55
#define VBE_WHITE       0x00FFFFFF

/* VBE color IDs matching VGA color constants for compatibility */
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

/* Default VBE colors mapped to 32-bit values */
#define VBE_DEFAULT_BG VBE_BLACK
#define VBE_DEFAULT_FG VBE_YELLOW

/** Standard VGA palette mapped to 32-bit RGBA */
extern const uint32_t vbe_palette[16];

/** Framebuffer state structure */
typedef struct {
    uint8_t  initialized;       /* 1 if framebuffer is active */
    uint8_t  fb_type;           /* Framebuffer type (RGB=1, text=2, indexed=3) */
    uint32_t width;             /* Width in pixels */
    uint32_t height;            /* Height in pixels */
    uint32_t pitch;             /* Bytes per scanline */
    uint8_t  bpp;               /* Bits per pixel (24 or 32) */
    uint32_t phys_addr;         /* Physical address of framebuffer */
    uint32_t virt_addr;         /* Virtual address (identity-mapped) */
    uint8_t  red_pos;           /* Red field position */
    uint8_t  red_size;          /* Red field size */
    uint8_t  green_pos;         /* Green field position */
    uint8_t  green_size;        /* Green field size */
    uint8_t  blue_pos;          /* Blue field position */
    uint8_t  blue_size;         /* Blue field size */
} vbe_fb_t;

/** Global framebuffer state */
extern vbe_fb_t vbe_fb;

/** Default foreground and background colors */
extern uint32_t vbe_fg_color;
extern uint32_t vbe_bg_color;

/* ============================================ */
/* Initialization                               */
/* ============================================ */

/**
 * Initialize VBE framebuffer from Multiboot information.
 * Must be called after paging is enabled so the framebuffer
 * memory (which may be above 4GB physical) can be mapped.
 *
 * @param mbi_flags          Multiboot info flags field
 * @param fb_addr            framebuffer_addr from MBI (physical)
 * @param fb_pitch           framebuffer_pitch from MBI
 * @param fb_width           framebuffer_width from MBI
 * @param fb_height          framebuffer_height from MBI
 * @param fb_bpp             framebuffer_bpp from MBI
 * @param fb_type            framebuffer_type from MBI
 * @param red_pos, red_size, green_pos, green_size, blue_pos, blue_size
 * @return 1 on success, 0 if no framebuffer available
 */
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

/**
 * Check if VBE graphics mode is active.
 * @return 1 if framebuffer is initialized, 0 if in text mode
 */
int vbe_is_active(void);

/* ============================================ */
/* Pixel Drawing                                */
/* ============================================ */

/**
 * Draw a single pixel at (x, y) with the specified color.
 * @param x X coordinate
 * @param y Y coordinate
 * @param color 32-bit RGBA color value
 */
void vbe_putpixel(uint32_t x, uint32_t y, uint32_t color);

/**
 * Get the pixel color at (x, y).
 * @param x X coordinate
 * @param y Y coordinate
 * @return 32-bit RGBA color value
 */
uint32_t vbe_getpixel(uint32_t x, uint32_t y);

/* ============================================ */
/* Shape Drawing                                */
/* ============================================ */

/**
 * Fill a rectangle with a solid color.
 * @param x      X coordinate of top-left corner
 * @param y      Y coordinate of top-left corner
 * @param w      Width of rectangle
 * @param h      Height of rectangle
 * @param color Fill color
 */
void vbe_fill_rect(uint32_t x, uint32_t y, uint32_t w, uint32_t h, uint32_t color);

/**
 * Draw a rectangle outline using the specified color.
 * @param x      X coordinate of top-left corner
 * @param y      Y coordinate of top-left corner
 * @param w      Width of rectangle
 * @param h      Height of rectangle
 * @param color Outline color
 * @param thickness Line thickness in pixels
 */
void vbe_draw_rect(uint32_t x, uint32_t y, uint32_t w, uint32_t h, uint32_t color, uint32_t thickness);

/**
 * Draw a horizontal line.
 * @param x      Start X coordinate
 * @param y      Y coordinate
 * @param len    Length in pixels
 * @param color  Line color
 */
void vbe_draw_hline(uint32_t x, uint32_t y, uint32_t len, uint32_t color);

/**
 * Draw a vertical line.
 * @param x      X coordinate
 * @param y      Start Y coordinate
 * @param len    Length in pixels
 * @param color  Line color
 */
void vbe_draw_vline(uint32_t x, uint32_t y, uint32_t len, uint32_t color);

/* ============================================ */
/* Screen Management                            */
/* ============================================ */

/**
 * Clear the entire screen with the background color.
 */
void vbe_clear(void);

/**
 * Clear the screen with a specific color.
 * @param color Fill color
 */
void vbe_clear_color(uint32_t color);

/**
 * Scroll the screen up by a number of pixel lines.
 * @param lines Number of pixels to scroll
 */
void vbe_scroll(uint32_t lines);

/* ============================================ */
/* Text Rendering                               */
/* ============================================ */

/**
 * Render a character at (x, y) using the 8x16 bitmap font.
 * @param x      X coordinate of top-left corner
 * @param y      Y coordinate of top-left corner
 * @param c      Character to render (ASCII 32-126)
 * @param fg     Foreground color
 * @param bg     Background color
 */
void vbe_draw_char(uint32_t x, uint32_t y, char c, uint32_t fg, uint32_t bg);

/**
 * Render a null-terminated string at (x, y).
 * Automatically wraps at screen width and handles newlines.
 *
 * @param x      Starting X coordinate
 * @param y      Starting Y coordinate
 * @param str    String to render
 * @param fg     Foreground color
 * @param bg     Background color
 */
void vbe_draw_string(uint32_t x, uint32_t y, const char *str, uint32_t fg, uint32_t bg);

/**
 * Render a formatted string with default colors.
 * @param x      Starting X coordinate
 * @param y      Starting Y coordinate
 * @param str    String to render
 */
void vbe_puts(uint32_t x, uint32_t y, const char *str);

/* ============================================ */
/* Color Conversion                             */
/* ============================================ */

/**
 * Convert a VGA color index (0-15) to a 32-bit RGB value.
 * @param vga_color VGA color index (0-15)
 * @return 32-bit RGBA color
 */
uint32_t vbe_vga_to_rgb(uint8_t vga_color);

/**
 * Convert RGB components to packed 32-bit pixel value.
 * @param r Red component (0-255)
 * @param g Green component (0-255)
 * @param b Blue component (0-255)
 * @return 32-bit RGBA color
 */
static inline uint32_t vbe_rgb(uint8_t r, uint8_t g, uint8_t b) {
    return (r << 16) | (g << 8) | b;
}

#endif /* VBE_H */

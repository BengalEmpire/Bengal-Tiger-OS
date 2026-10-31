/**
 * Bengal Tiger OS - VBE Framebuffer Driver Implementation
 *
 * @file vbe.c
 * @author Bengal Tiger OS (BengalEmpire)
 * @version 1.0.0
 */

#include "vbe.h"
#include "font.h"
#include "common.h"
#include "paging.h"

vbe_fb_t vbe_fb;
uint32_t vbe_fg_color = VBE_YELLOW;
uint32_t vbe_bg_color = VBE_BLACK;

const uint32_t vbe_palette[16] = {
    0x000000, 0x0000AA, 0x00AA00, 0x00AAAA,
    0xAA0000, 0xAA00AA, 0xAA5500, 0xAAAAAA,
    0x555555, 0x5555FF, 0x55FF55, 0x55FFFF,
    0xFF5555, 0xFF55FF, 0xFFFF55, 0xFFFFFF
};

static inline uint32_t pack_hw_color(uint32_t rgb) {
    if (vbe_fb.red_pos == 16 && vbe_fb.green_pos == 8 && vbe_fb.blue_pos == 0) {
        return rgb;
    }
    uint32_t r = (rgb >> 16) & 0xFF;
    uint32_t g = (rgb >> 8)  & 0xFF;
    uint32_t b =  rgb        & 0xFF;
    return (r << vbe_fb.red_pos) | (g << vbe_fb.green_pos) | (b << vbe_fb.blue_pos);
}

static inline uint32_t unpack_hw_color(uint32_t hw) {
    if (vbe_fb.red_pos == 16 && vbe_fb.green_pos == 8 && vbe_fb.blue_pos == 0) {
        return hw & 0x00FFFFFFU;
    }
    uint32_t r = (hw >> vbe_fb.red_pos)   & 0xFF;
    uint32_t g = (hw >> vbe_fb.green_pos) & 0xFF;
    uint32_t b = (hw >> vbe_fb.blue_pos)  & 0xFF;
    return (r << 16) | (g << 8) | b;
}

int vbe_init(uint32_t mbi_flags,
             uint64_t fb_addr,
             uint32_t fb_pitch,
             uint32_t fb_width,
             uint32_t fb_height,
             uint8_t  fb_bpp,
             uint8_t  fb_type,
             uint8_t  red_pos, uint8_t red_size,
             uint8_t  green_pos, uint8_t green_size,
             uint8_t  blue_pos, uint8_t blue_size) {

    memset(&vbe_fb, 0, sizeof(vbe_fb));

    if (!(mbi_flags & (1U << 12))) return 0;
    if (fb_type != FB_TYPE_RGB) return 0;
    if (fb_bpp != 24 && fb_bpp != 32) return 0;
    if (fb_addr == 0 || fb_addr > 0xFFFF0000ULL) return 0;
    if (fb_width == 0 || fb_height == 0 || fb_pitch == 0) return 0;

    vbe_fb.initialized = 1;
    vbe_fb.fb_type     = fb_type;
    vbe_fb.width       = fb_width;
    vbe_fb.height      = fb_height;
    vbe_fb.pitch       = fb_pitch;
    vbe_fb.bpp         = fb_bpp;
    vbe_fb.phys_addr   = (uint32_t)fb_addr;
    vbe_fb.virt_addr   = (uint32_t)fb_addr;
    vbe_fb.red_pos     = red_size   ? red_pos   : 16;
    vbe_fb.red_size    = red_size   ? red_size  : 8;
    vbe_fb.green_pos   = green_size ? green_pos : 8;
    vbe_fb.green_size  = green_size ? green_size: 8;
    vbe_fb.blue_pos    = blue_size  ? blue_pos  : 0;
    vbe_fb.blue_size   = blue_size  ? blue_size : 8;

    uint32_t fb_size = fb_pitch * fb_height;
    paging_map_region(vbe_fb.virt_addr, vbe_fb.phys_addr, fb_size,
                      PAGE_PRESENT | PAGE_WRITABLE);

    vbe_clear();
    return 1;
}

int vbe_is_active(void) {
    return vbe_fb.initialized;
}

void vbe_putpixel(uint32_t x, uint32_t y, uint32_t color) {
    if (!vbe_fb.initialized || x >= vbe_fb.width || y >= vbe_fb.height) return;

    uint32_t hw = pack_hw_color(color);
    uint8_t *base = (uint8_t*)vbe_fb.virt_addr;

    if (vbe_fb.bpp == 32) {
        uint32_t *row = (uint32_t*)(base + y * vbe_fb.pitch);
        row[x] = hw;
    } else if (vbe_fb.bpp == 24) {
        uint8_t *p = base + y * vbe_fb.pitch + x * 3;
        p[0] = (uint8_t)(hw & 0xFF);
        p[1] = (uint8_t)((hw >> 8) & 0xFF);
        p[2] = (uint8_t)((hw >> 16) & 0xFF);
    }
}

uint32_t vbe_getpixel(uint32_t x, uint32_t y) {
    if (!vbe_fb.initialized || x >= vbe_fb.width || y >= vbe_fb.height) return 0;

    uint8_t *base = (uint8_t*)vbe_fb.virt_addr;
    if (vbe_fb.bpp == 32) {
        uint32_t *row = (uint32_t*)(base + y * vbe_fb.pitch);
        return unpack_hw_color(row[x]);
    } else if (vbe_fb.bpp == 24) {
        uint8_t *p = base + y * vbe_fb.pitch + x * 3;
        uint32_t hw = (uint32_t)p[0] | ((uint32_t)p[1] << 8) | ((uint32_t)p[2] << 16);
        return unpack_hw_color(hw);
    }
    return 0;
}

void vbe_fill_rect(uint32_t x, uint32_t y, uint32_t w, uint32_t h, uint32_t color) {
    if (!vbe_fb.initialized || w == 0 || h == 0) return;
    if (x >= vbe_fb.width || y >= vbe_fb.height) return;
    if (x + w > vbe_fb.width)  w = vbe_fb.width - x;
    if (y + h > vbe_fb.height) h = vbe_fb.height - y;

    if (vbe_fb.bpp == 32) {
        uint32_t hw = pack_hw_color(color);
        uint8_t *base = (uint8_t*)vbe_fb.virt_addr;
        for (uint32_t row = 0; row < h; row++) {
            uint32_t *dst = (uint32_t*)(base + (y + row) * vbe_fb.pitch) + x;
            for (uint32_t col = 0; col < w; col++) {
                dst[col] = hw;
            }
        }
    } else {
        for (uint32_t row = 0; row < h; row++) {
            for (uint32_t col = 0; col < w; col++) {
                vbe_putpixel(x + col, y + row, color);
            }
        }
    }
}

void vbe_draw_rect(uint32_t x, uint32_t y, uint32_t w, uint32_t h, uint32_t color, uint32_t thickness) {
    if (!vbe_fb.initialized || thickness == 0 || w == 0 || h == 0) return;
    if (thickness > h / 2) thickness = h / 2;
    if (thickness > w / 2) thickness = w / 2;
    if (thickness == 0) thickness = 1;

    vbe_fill_rect(x, y, w, thickness, color);
    if (h > thickness) {
        vbe_fill_rect(x, y + h - thickness, w, thickness, color);
    }
    if (h > 2 * thickness) {
        vbe_fill_rect(x, y + thickness, thickness, h - 2 * thickness, color);
        if (w > thickness) {
            vbe_fill_rect(x + w - thickness, y + thickness, thickness, h - 2 * thickness, color);
        }
    }
}

void vbe_draw_hline(uint32_t x, uint32_t y, uint32_t len, uint32_t color) {
    vbe_fill_rect(x, y, len, 1, color);
}

void vbe_draw_vline(uint32_t x, uint32_t y, uint32_t len, uint32_t color) {
    vbe_fill_rect(x, y, 1, len, color);
}

void vbe_clear(void) {
    vbe_clear_color(vbe_bg_color);
}

void vbe_clear_color(uint32_t color) {
    if (!vbe_fb.initialized) return;
    vbe_fill_rect(0, 0, vbe_fb.width, vbe_fb.height, color);
}

void vbe_scroll_region(uint32_t top_y, uint32_t region_h, uint32_t lines, uint32_t bg_color) {
    if (!vbe_fb.initialized || lines == 0 || region_h == 0) return;
    if (top_y >= vbe_fb.height) return;
    if (top_y + region_h > vbe_fb.height) {
        region_h = vbe_fb.height - top_y;
    }
    if (lines >= region_h) {
        vbe_fill_rect(0, top_y, vbe_fb.width, region_h, bg_color);
        return;
    }

    uint8_t *base = (uint8_t*)vbe_fb.virt_addr;
    uint32_t copy_lines = region_h - lines;
    memcpy(base + top_y * vbe_fb.pitch,
           base + (top_y + lines) * vbe_fb.pitch,
           copy_lines * vbe_fb.pitch);

    vbe_fill_rect(0, top_y + copy_lines, vbe_fb.width, lines, bg_color);
}

void vbe_scroll(uint32_t lines) {
    vbe_scroll_region(0, vbe_fb.height, lines, vbe_bg_color);
}

void vbe_draw_char(uint32_t x, uint32_t y, char c, uint32_t fg, uint32_t bg) {
    if (!vbe_fb.initialized) return;
    if (x + FONT_WIDTH > vbe_fb.width || y + FONT_HEIGHT > vbe_fb.height) return;

    int index = (int)(uint8_t)c - FONT_FIRST_CHAR;
    if (index < 0 || index >= FONT_NUM_CHARS) {
        index = 0; /* Render non-printable / control glyph slot as space */
    }

    const uint8_t *glyph = font_8x16[index];

    if (vbe_fb.bpp == 32) {
        uint32_t hw_fg = pack_hw_color(fg);
        uint32_t hw_bg = pack_hw_color(bg);
        uint8_t *base  = (uint8_t*)vbe_fb.virt_addr;

        for (uint32_t row = 0; row < FONT_HEIGHT; row++) {
            uint8_t bits = glyph[row];
            uint32_t *dst = (uint32_t*)(base + (y + row) * vbe_fb.pitch) + x;
            dst[0] = (bits & 0x80) ? hw_fg : hw_bg;
            dst[1] = (bits & 0x40) ? hw_fg : hw_bg;
            dst[2] = (bits & 0x20) ? hw_fg : hw_bg;
            dst[3] = (bits & 0x10) ? hw_fg : hw_bg;
            dst[4] = (bits & 0x08) ? hw_fg : hw_bg;
            dst[5] = (bits & 0x04) ? hw_fg : hw_bg;
            dst[6] = (bits & 0x02) ? hw_fg : hw_bg;
            dst[7] = (bits & 0x01) ? hw_fg : hw_bg;
        }
    } else {
        for (uint32_t row = 0; row < FONT_HEIGHT; row++) {
            uint8_t bits = glyph[row];
            for (uint32_t col = 0; col < FONT_WIDTH; col++) {
                vbe_putpixel(x + col, y + row, (bits & (1U << (7 - col))) ? fg : bg);
            }
        }
    }
}

void vbe_draw_string(uint32_t x, uint32_t y, const char *str, uint32_t fg, uint32_t bg) {
    if (!vbe_fb.initialized || !str) return;

    uint32_t cur_x = x;
    uint32_t cur_y = y;

    while (*str) {
        if (*str == '\n') {
            cur_x = x;
            cur_y += FONT_HEIGHT;
            if (cur_y + FONT_HEIGHT > vbe_fb.height) {
                vbe_scroll(FONT_HEIGHT);
                cur_y -= FONT_HEIGHT;
            }
            str++;
            continue;
        }
        if (*str == '\r') {
            cur_x = x;
            str++;
            continue;
        }

        vbe_draw_char(cur_x, cur_y, *str, fg, bg);
        cur_x += FONT_WIDTH;

        if (cur_x + FONT_WIDTH > vbe_fb.width) {
            cur_x = x;
            cur_y += FONT_HEIGHT;
            if (cur_y + FONT_HEIGHT > vbe_fb.height) {
                vbe_scroll(FONT_HEIGHT);
                cur_y -= FONT_HEIGHT;
            }
        }
        str++;
    }
}

void vbe_puts(uint32_t x, uint32_t y, const char *str) {
    vbe_draw_string(x, y, str, vbe_fg_color, vbe_bg_color);
}

uint32_t vbe_vga_to_rgb(uint8_t vga_color) {
    return (vga_color < 16) ? vbe_palette[vga_color] : VBE_WHITE;
}

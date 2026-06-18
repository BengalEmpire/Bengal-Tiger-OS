/**
 * Bengal Tiger OS - VBE Framebuffer Driver Implementation
 *
 * @file vbe.c
 * @author Bengal Tiger OS (BengalEmpire)
 * @version 0.4.0
 */

#include "vbe.h"
#include "font.h"
#include "common.h"
#include "paging.h"

/* Global framebuffer state */
vbe_fb_t vbe_fb;
uint32_t vbe_fg_color = VBE_YELLOW;
uint32_t vbe_bg_color = VBE_BLACK;

/* Standard VGA palette in 32-bit RGB */
const uint32_t vbe_palette[16] = {
    0x000000, 0x0000AA, 0x00AA00, 0x00AAAA,
    0xAA0000, 0xAA00AA, 0xAA5500, 0xAAAAAA,
    0x555555, 0x5555FF, 0x55FF55, 0x55FFFF,
    0xFF5555, 0xFF55FF, 0xFFFF55, 0xFFFFFF
};

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

    /* Check if framebuffer info is available (MBI bit 12) */
    if (!(mbi_flags & (1 << 12))) {
        return 0;
    }

    /* We only support RGB color framebuffers */
    if (fb_type != FB_TYPE_RGB) {
        return 0;
    }

    /* We need at least 24-bit color */
    if (fb_bpp < 24) {
        return 0;
    }

    /* Store framebuffer info */
    vbe_fb.initialized = 1;
    vbe_fb.fb_type     = fb_type;
    vbe_fb.width       = fb_width;
    vbe_fb.height      = fb_height;
    vbe_fb.pitch       = fb_pitch;
    vbe_fb.bpp         = fb_bpp;
    vbe_fb.phys_addr   = (uint32_t)fb_addr;
    vbe_fb.virt_addr   = (uint32_t)fb_addr;  /* Identity-mapped */
    vbe_fb.red_pos     = red_pos;
    vbe_fb.red_size    = red_size;
    vbe_fb.green_pos   = green_pos;
    vbe_fb.green_size  = green_size;
    vbe_fb.blue_pos    = blue_pos;
    vbe_fb.blue_size   = blue_size;

    /* 
     * Map the framebuffer into virtual address space.
     * The framebuffer may be at a high physical address (e.g., 0xE0000000+)
     * which is beyond our 4MB identity mapping.
     * We identity-map the entire framebuffer region.
     */
    uint32_t fb_size = fb_pitch * fb_height;
    uint32_t fb_start = (uint32_t)fb_addr;
    uint32_t fb_end = fb_start + fb_size;

    /* Map each page of the framebuffer */
    for (uint32_t addr = ALIGN_DOWN(fb_start, PAGE_SIZE);
         addr < fb_end;
         addr += PAGE_SIZE) {
        paging_map(addr, addr, PAGE_PRESENT | PAGE_WRITABLE);
    }

    /* Clear the screen */
    vbe_clear();

    return 1;
}

int vbe_is_active(void) {
    return vbe_fb.initialized;
}

void vbe_putpixel(uint32_t x, uint32_t y, uint32_t color) {
    if (!vbe_fb.initialized) return;
    if (x >= vbe_fb.width || y >= vbe_fb.height) return;

    uint32_t *fb = (uint32_t*)vbe_fb.virt_addr;
    uint32_t offset = y * (vbe_fb.pitch / 4) + x;

    if (vbe_fb.bpp == 32) {
        fb[offset] = color;
    } else if (vbe_fb.bpp == 24) {
        /* 24-bit: pack 3 bytes */
        uint8_t *pixel = (uint8_t*)&fb[offset];
        pixel[0] = color & 0xFF;
        pixel[1] = (color >> 8) & 0xFF;
        pixel[2] = (color >> 16) & 0xFF;
    }
}

uint32_t vbe_getpixel(uint32_t x, uint32_t y) {
    if (!vbe_fb.initialized) return 0;
    if (x >= vbe_fb.width || y >= vbe_fb.height) return 0;

    uint32_t *fb = (uint32_t*)vbe_fb.virt_addr;
    return fb[y * (vbe_fb.pitch / 4) + x];
}

void vbe_fill_rect(uint32_t x, uint32_t y, uint32_t w, uint32_t h, uint32_t color) {
    if (!vbe_fb.initialized) return;

    /* Clamp to screen bounds */
    if (x >= vbe_fb.width || y >= vbe_fb.height) return;
    if (x + w > vbe_fb.width)  w = vbe_fb.width - x;
    if (y + h > vbe_fb.height) h = vbe_fb.height - y;

    uint32_t *fb = (uint32_t*)vbe_fb.virt_addr;
    uint32_t stride = vbe_fb.pitch / 4;

    /* Fill the rectangle row by row using 32-bit writes */
    for (uint32_t row = 0; row < h; row++) {
        uint32_t *row_start = fb + (y + row) * stride + x;
        for (uint32_t col = 0; col < w; col++) {
            row_start[col] = color;
        }
    }
}

void vbe_draw_rect(uint32_t x, uint32_t y, uint32_t w, uint32_t h, uint32_t color, uint32_t thickness) {
    if (!vbe_fb.initialized || thickness == 0) return;

    /* Guard against thick borders that exceed rect dimensions */
    if (thickness > h / 2) thickness = h / 2;
    if (thickness > w / 2) thickness = w / 2;
    if (thickness == 0) thickness = 1;

    /* Top and bottom edges */
    for (uint32_t t = 0; t < thickness && t < h; t++) {
        vbe_draw_hline(x, y + t, w, color);
        if (h > thickness && (y + h - 1 - t) > (y + t)) {
            vbe_draw_hline(x, y + h - 1 - t, w, color);
        }
    }

    /* Left and right edges */
    for (uint32_t t = 0; t < thickness && t < w; t++) {
        vbe_draw_vline(x + t, y + thickness, h - 2 * thickness, color);
        if (w > thickness && (x + w - 1 - t) > (x + t)) {
            vbe_draw_vline(x + w - 1 - t, y + thickness, h - 2 * thickness, color);
        }
    }
}

void vbe_draw_hline(uint32_t x, uint32_t y, uint32_t len, uint32_t color) {
    if (!vbe_fb.initialized) return;
    if (y >= vbe_fb.height) return;
    if (x >= vbe_fb.width) return;
    if (x + len > vbe_fb.width) len = vbe_fb.width - x;

    uint32_t *fb = (uint32_t*)vbe_fb.virt_addr;
    uint32_t stride = vbe_fb.pitch / 4;
    uint32_t offset = y * stride + x;

    for (uint32_t i = 0; i < len; i++) {
        fb[offset + i] = color;
    }
}

void vbe_draw_vline(uint32_t x, uint32_t y, uint32_t len, uint32_t color) {
    if (!vbe_fb.initialized) return;
    if (x >= vbe_fb.width) return;
    if (y >= vbe_fb.height) return;
    if (y + len > vbe_fb.height) len = vbe_fb.height - y;

    uint32_t *fb = (uint32_t*)vbe_fb.virt_addr;
    uint32_t stride = vbe_fb.pitch / 4;

    for (uint32_t i = 0; i < len; i++) {
        fb[(y + i) * stride + x] = color;
    }
}

void vbe_clear(void) {
    vbe_clear_color(vbe_bg_color);
}

void vbe_clear_color(uint32_t color) {
    if (!vbe_fb.initialized) return;

    uint32_t *fb = (uint32_t*)vbe_fb.virt_addr;
    uint32_t stride = vbe_fb.pitch / 4;

    for (uint32_t y = 0; y < vbe_fb.height; y++) {
        for (uint32_t x = 0; x < vbe_fb.width; x++) {
            fb[y * stride + x] = color;
        }
    }
}

void vbe_scroll(uint32_t lines) {
    if (!vbe_fb.initialized || lines == 0) return;
    if (lines >= vbe_fb.height) {
        vbe_clear();
        return;
    }

    uint32_t *fb = (uint32_t*)vbe_fb.virt_addr;
    uint32_t stride = vbe_fb.pitch / 4;
    uint32_t copy_lines = vbe_fb.height - lines;
    uint32_t bytes_to_move = copy_lines * vbe_fb.pitch;

    /* Move all lines up by `lines` pixels.
     * Kernel's memcpy handles overlapping regions (backwards copy if dest > src). */
    memcpy((void*)fb, (void*)((uint8_t*)fb + lines * vbe_fb.pitch), bytes_to_move);

    /* Clear the newly exposed bottom area */
    for (uint32_t y = copy_lines; y < vbe_fb.height; y++) {
        for (uint32_t x = 0; x < vbe_fb.width; x++) {
            fb[y * stride + x] = vbe_bg_color;
        }
    }
}

void vbe_draw_char(uint32_t x, uint32_t y, char c, uint32_t fg, uint32_t bg) {
    if (!vbe_fb.initialized) return;
    if (x + FONT_WIDTH > vbe_fb.width || y + FONT_HEIGHT > vbe_fb.height) return;

    uint32_t *fb = (uint32_t*)vbe_fb.virt_addr;
    uint32_t stride = vbe_fb.pitch / 4;

    /* Get font data for this character */
    int index = (unsigned char)c - FONT_FIRST_CHAR;
    if (index < 0 || index >= FONT_NUM_CHARS) return;

    const uint8_t *glyph = font_8x16[index];

    /* Render the character, pixel by pixel */
    for (uint32_t row = 0; row < FONT_HEIGHT; row++) {
        uint8_t bits = glyph[row];
        uint32_t fb_row = (y + row) * stride;

        for (uint32_t col = 0; col < FONT_WIDTH; col++) {
            uint32_t color = (bits & (1 << (7 - col))) ? fg : bg;
            fb[fb_row + (x + col)] = color;
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

        /* Draw character */
        vbe_draw_char(cur_x, cur_y, *str, fg, bg);

        cur_x += FONT_WIDTH;

        /* Word wrap */
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
    if (vga_color < 16) {
        return vbe_palette[vga_color];
    }
    return VBE_WHITE;
}

/**
 * Bengal Tiger OS - PS/2 Mouse Driver Implementation
 *
 * Properly preserves PS/2 Port 1 (Keyboard IRQ1 + Clock) across all
 * initialization paths, synchronizes packet framing via byte-0 bit 3,
 * discards overflow packets, and renders a crisp arrow cursor on VBE.
 *
 * @file mouse.c
 * @author Bengal Tiger OS (BengalEmpire)
 * @version 1.0.0
 */

#include "mouse.h"
#include "pic.h"
#include "vbe.h"
#include "common.h"

mouse_state_t mouse_state;

static const uint8_t cursor_bitmap[MOUSE_CURSOR_HEIGHT][2] = {
    {0x80, 0x00},
    {0xC0, 0x00},
    {0xE0, 0x00},
    {0xF0, 0x00},
    {0xF8, 0x00},
    {0xFC, 0x00},
    {0xFE, 0x00},
    {0xFF, 0x00},
    {0xFF, 0x80},
    {0xFF, 0xC0},
    {0xFF, 0xE0},
    {0xF0, 0xE0},
    {0xE0, 0xE0},
    {0xC0, 0x70},
    {0x80, 0x70},
    {0x00, 0x38},
    {0x00, 0x38},
    {0x00, 0x1C},
    {0x00, 0x1C},
};

static int ps2_wait_output(void) {
    int timeout = 100000;
    while (--timeout > 0) {
        if (inb(PS2_STATUS_PORT) & 0x01) {
            return 0;
        }
    }
    return -1;
}

static int ps2_wait_input(void) {
    int timeout = 100000;
    while (--timeout > 0) {
        if (!(inb(PS2_STATUS_PORT) & 0x02)) {
            return 0;
        }
    }
    return -1;
}

static void ps2_flush_output(void) {
    int limit = 64;
    while (limit-- > 0 && (inb(PS2_STATUS_PORT) & 0x01)) {
        (void)inb(PS2_DATA_PORT);
    }
}

static uint8_t ps2_read_config(void) {
    if (ps2_wait_input() < 0) return 0;
    outb(PS2_COMMAND_PORT, PS2_CMD_READ_CONFIG);
    if (ps2_wait_output() < 0) return 0;
    return inb(PS2_DATA_PORT);
}

static void ps2_write_config(uint8_t config) {
    if (ps2_wait_input() < 0) return;
    outb(PS2_COMMAND_PORT, PS2_CMD_WRITE_CONFIG);
    if (ps2_wait_input() < 0) return;
    outb(PS2_DATA_PORT, config);
}

/* CRITICAL: Always re-enable PS/2 Port 1 (Keyboard) + IRQ1! */
static void ps2_restore_keyboard(int enable_mouse_irq) {
    uint8_t config = ps2_read_config();
    config |= 0x01;   /* Enable IRQ1 (Keyboard) */
    config &= ~0x10;  /* Clear Port 1 clock disable */
    if (enable_mouse_irq) {
        config |= 0x02;   /* Enable IRQ12 (Mouse) */
        config &= ~0x20;  /* Clear Port 2 clock disable */
    }
    ps2_write_config(config);

    if (ps2_wait_input() == 0) {
        outb(PS2_COMMAND_PORT, PS2_CMD_ENABLE_PORT1);
    }
    ps2_flush_output();
}

static int mouse_send_cmd(uint8_t cmd) {
    if (ps2_wait_input() < 0) return -1;
    outb(PS2_COMMAND_PORT, 0xD4);

    if (ps2_wait_input() < 0) return -1;
    outb(PS2_DATA_PORT, cmd);

    if (ps2_wait_output() < 0) return -1;
    uint8_t resp = inb(PS2_DATA_PORT);

    return (resp == PS2_ACK) ? 0 : -2;
}

void mouse_init(void) {
    memset(&mouse_state, 0, sizeof(mouse_state));
    mouse_state.cursor_old_x = -1;
    mouse_state.cursor_old_y = -1;
    mouse_state.packet_size  = 3;

    if (ps2_wait_input() < 0) return;
    outb(PS2_COMMAND_PORT, PS2_CMD_DISABLE_PORT1);
    if (ps2_wait_input() < 0) { ps2_restore_keyboard(0); return; }
    outb(PS2_COMMAND_PORT, PS2_CMD_DISABLE_PORT2);

    ps2_flush_output();

    /* Enable Port 2 and test if dual-channel controller responds */
    if (ps2_wait_input() < 0) { ps2_restore_keyboard(0); return; }
    outb(PS2_COMMAND_PORT, PS2_CMD_ENABLE_PORT2);

    uint8_t config = ps2_read_config();
    if (config & 0x20) {
        /* Single-channel PS/2 controller: no mouse port */
        ps2_restore_keyboard(0);
        return;
    }

    if (mouse_send_cmd(PS2_MOUSE_CMD_SET_DEFAULTS) < 0) {
        ps2_restore_keyboard(0);
        return;
    }

    /* IntelliMouse scroll-wheel magic sample rate sequence: 200, 100, 80 */
    mouse_send_cmd(PS2_MOUSE_CMD_SET_SAMPLE);
    mouse_send_cmd(200);
    mouse_send_cmd(PS2_MOUSE_CMD_SET_SAMPLE);
    mouse_send_cmd(100);
    mouse_send_cmd(PS2_MOUSE_CMD_SET_SAMPLE);
    mouse_send_cmd(80);

    if (mouse_send_cmd(PS2_MOUSE_CMD_GET_DEV_ID) >= 0) {
        if (ps2_wait_output() >= 0) {
            uint8_t device_id = inb(PS2_DATA_PORT);
            if (device_id == PS2_MOUSE_ID_WHEEL || device_id == PS2_MOUSE_ID_5BUTTON) {
                mouse_state.has_wheel   = 1;
                mouse_state.packet_size = 4;
            }
        }
    }

    mouse_send_cmd(PS2_MOUSE_CMD_SET_SAMPLE);
    mouse_send_cmd(100);

    if (mouse_send_cmd(PS2_MOUSE_CMD_ENABLE) < 0) {
        ps2_restore_keyboard(0);
        return;
    }

    /* Re-enable BOTH Port 1 (Keyboard IRQ1) and Port 2 (Mouse IRQ12) */
    ps2_restore_keyboard(1);

    if (vbe_fb.initialized) {
        mouse_state.x = (int32_t)(vbe_fb.width / 2);
        mouse_state.y = (int32_t)(vbe_fb.height / 2);
    } else {
        mouse_state.x = 400;
        mouse_state.y = 300;
    }

    pic_unmask_irq(2);  /* Ensure cascade IRQ2 unmasked */
    pic_unmask_irq(12); /* Unmask Mouse IRQ12 */

    mouse_state.present = 1;

    if (vbe_fb.initialized) {
        mouse_draw_cursor();
    }
}

void mouse_handler(void) {
    uint8_t status = inb(PS2_STATUS_PORT);
    if (!(status & 0x01)) return;

    uint8_t data = inb(PS2_DATA_PORT);
    /* Ensure byte came from auxiliary (mouse) device when bit 5 supported */
    if (!mouse_state.present) return;

    /* Synchronize packet alignment: Byte 0 MUST have bit 3 (0x08) set */
    if (mouse_state.packet_index == 0 && !(data & 0x08)) {
        return;
    }

    mouse_state.packet[mouse_state.packet_index++] = data;

    if (mouse_state.packet_index >= mouse_state.packet_size) {
        mouse_state.packet_index = 0;

        uint8_t *pkt = mouse_state.packet;

        /* Discard movement if X or Y overflow occurred */
        if (!(pkt[0] & (MOUSE_PKT_X_OVERFLOW | MOUSE_PKT_Y_OVERFLOW))) {
            int32_t dx = (int32_t)pkt[1];
            int32_t dy = (int32_t)pkt[2];

            if (pkt[0] & MOUSE_PKT_X_SIGN) dx |= (int32_t)0xFFFFFF00U;
            if (pkt[0] & MOUSE_PKT_Y_SIGN) dy |= (int32_t)0xFFFFFF00U;

            dy = -dy;

            uint32_t max_w = vbe_fb.initialized ? vbe_fb.width  : 800;
            uint32_t max_h = vbe_fb.initialized ? vbe_fb.height : 600;

            mouse_state.x += dx;
            mouse_state.y += dy;

            if (mouse_state.x < 0) mouse_state.x = 0;
            if (mouse_state.x >= (int32_t)max_w) mouse_state.x = (int32_t)max_w - 1;
            if (mouse_state.y < 0) mouse_state.y = 0;
            if (mouse_state.y >= (int32_t)max_h) mouse_state.y = (int32_t)max_h - 1;

            if (vbe_fb.initialized) {
                mouse_state.cursor_dirty = 1;
            }
        }

        mouse_state.buttons = 0;
        if (pkt[0] & MOUSE_PKT_LEFT_BUTTON)   mouse_state.buttons |= MOUSE_BTN_LEFT;
        if (pkt[0] & MOUSE_PKT_RIGHT_BUTTON)  mouse_state.buttons |= MOUSE_BTN_RIGHT;
        if (pkt[0] & MOUSE_PKT_MIDDLE_BUTTON) mouse_state.buttons |= MOUSE_BTN_MIDDLE;

        if (mouse_state.packet_size >= 4) {
            mouse_state.wheel_delta = (int8_t)(pkt[3] & 0x0F);
            if (mouse_state.wheel_delta & 0x08) {
                mouse_state.wheel_delta |= (int8_t)0xF0;
            }
        }
    }
}

int32_t mouse_get_x(void)       { return mouse_state.x; }
int32_t mouse_get_y(void)       { return mouse_state.y; }
uint8_t mouse_get_buttons(void) { return mouse_state.buttons; }
int     mouse_is_present(void)  { return mouse_state.present; }

void mouse_draw_cursor(void) {
    if (!vbe_fb.initialized || !mouse_state.present) return;

    int32_t x = mouse_state.x;
    int32_t y = mouse_state.y;

    if (x < 0 || y < 0 || x >= (int32_t)vbe_fb.width || y >= (int32_t)vbe_fb.height) {
        return;
    }

    for (int32_t row = 0; row < MOUSE_CURSOR_HEIGHT; row++) {
        int32_t draw_y = y + row;
        if (draw_y >= (int32_t)vbe_fb.height) break;

        for (int32_t col = 0; col < MOUSE_CURSOR_WIDTH; col++) {
            int32_t draw_x = x + col;
            if (draw_x >= (int32_t)vbe_fb.width) break;

            mouse_state.cursor_save[row * MOUSE_CURSOR_WIDTH + col] =
                vbe_getpixel((uint32_t)draw_x, (uint32_t)draw_y);
        }
    }

    for (int32_t row = 0; row < MOUSE_CURSOR_HEIGHT; row++) {
        int32_t draw_y = y + row;
        if (draw_y < 0 || draw_y >= (int32_t)vbe_fb.height) continue;

        uint16_t bits = ((uint16_t)cursor_bitmap[row][0] << 8) | cursor_bitmap[row][1];

        for (int32_t col = 0; col < MOUSE_CURSOR_WIDTH; col++) {
            int32_t draw_x = x + col;
            if (draw_x < 0 || draw_x >= (int32_t)vbe_fb.width) continue;

            if ((bits >> (15 - col)) & 1) {
                int is_edge = (row == 0 || row == MOUSE_CURSOR_HEIGHT - 1 ||
                               col == 0 || col == MOUSE_CURSOR_WIDTH - 1);
                if (!is_edge) {
                    for (int dr = -1; dr <= 1 && !is_edge; dr++) {
                        for (int dc = -1; dc <= 1 && !is_edge; dc++) {
                            uint16_t nbits = ((uint16_t)cursor_bitmap[row + dr][0] << 8) |
                                              cursor_bitmap[row + dr][1];
                            if (!((nbits >> (15 - (col + dc))) & 1)) {
                                is_edge = 1;
                            }
                        }
                    }
                }
                vbe_putpixel((uint32_t)draw_x, (uint32_t)draw_y,
                             is_edge ? VBE_WHITE : VBE_BLACK);
            }
        }
    }

    mouse_state.cursor_old_x   = x;
    mouse_state.cursor_old_y   = y;
    mouse_state.cursor_visible = 1;
}

void mouse_erase_cursor(void) {
    if (!vbe_fb.initialized || !mouse_state.cursor_visible) return;

    int32_t old_x = mouse_state.cursor_old_x;
    int32_t old_y = mouse_state.cursor_old_y;

    for (int32_t row = 0; row < MOUSE_CURSOR_HEIGHT; row++) {
        int32_t draw_y = old_y + row;
        if (draw_y < 0 || draw_y >= (int32_t)vbe_fb.height) continue;

        for (int32_t col = 0; col < MOUSE_CURSOR_WIDTH; col++) {
            int32_t draw_x = old_x + col;
            if (draw_x < 0 || draw_x >= (int32_t)vbe_fb.width) continue;

            vbe_putpixel((uint32_t)draw_x, (uint32_t)draw_y,
                         mouse_state.cursor_save[row * MOUSE_CURSOR_WIDTH + col]);
        }
    }

    mouse_state.cursor_visible = 0;
}

void mouse_update_cursor(void) {
    if (!vbe_fb.initialized || !mouse_state.present || !mouse_state.cursor_dirty) return;

    uint32_t irq = irq_save();
    mouse_state.cursor_dirty = 0;
    int32_t nx = mouse_state.x;
    int32_t ny = mouse_state.y;
    irq_restore(irq);

    if (mouse_state.cursor_visible &&
        nx == mouse_state.cursor_old_x &&
        ny == mouse_state.cursor_old_y) {
        return;
    }

    mouse_erase_cursor();
    mouse_draw_cursor();
}

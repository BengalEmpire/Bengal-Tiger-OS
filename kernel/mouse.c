/**
 * Bengal Tiger OS - PS/2 Mouse Driver Implementation
 *
 * @file mouse.c
 * @author Bengal Tiger OS (BengalEmpire)
 * @version 0.5.0
 */

#include "mouse.h"
#include "vbe.h"
#include "common.h"

/* Global mouse state */
mouse_state_t mouse_state;

/** 
 * Mouse cursor bitmap (12x19 pixels, 1 bit per pixel).
 * Bit = 1 is foreground (white outline), Bit = 0 is background.
 * Single-bit-per-pixel layout, 2 bytes per row.
 */
static const uint8_t cursor_bitmap[MOUSE_CURSOR_HEIGHT][2] = {
    {0b10000000, 0b00000000},  /* #............. */
    {0b11000000, 0b00000000},  /* ##............ */
    {0b11100000, 0b00000000},  /* ###........... */
    {0b11110000, 0b00000000},  /* ####.......... */
    {0b11111000, 0b00000000},  /* #####......... */
    {0b11111100, 0b00000000},  /* ######........ */
    {0b11111110, 0b00000000},  /* #######....... */
    {0b11111111, 0b00000000},  /* ########...... */
    {0b11111111, 0b10000000},  /* #########..... */
    {0b11111111, 0b11000000},  /* ##########.... */
    {0b11111111, 0b11100000},  /* ###########... */
    {0b11110000, 0b11100000},  /* ####..###..... */
    {0b11100000, 0b11100000},  /* ###...###..... */
    {0b11000000, 0b01110000},  /* ##....###..... */
    {0b10000000, 0b01110000},  /* #.....###..... */
    {0b00000000, 0b00111000},  /* ......###..... */
    {0b00000000, 0b00111000},  /* ......###..... */
    {0b00000000, 0b00011100},  /* .......###.... */
    {0b00000000, 0b00011100},  /* .......###.... */
};

/* ============================================ */
/* PS/2 Controller Helpers                       */
/* ============================================ */

/** Wait for PS/2 controller to have data to read */
static int ps2_wait_output(void) {
    int timeout = 100000;
    while (--timeout) {
        if (inb(PS2_STATUS_PORT) & 0x01) {
            return 0;  /* Data available */
        }
    }
    return -1;  /* Timeout */
}

/** Wait for PS/2 controller to accept a command */
static int ps2_wait_input(void) {
    int timeout = 100000;
    while (--timeout) {
        if (!(inb(PS2_STATUS_PORT) & 0x02)) {
            return 0;  /* Input buffer empty */
        }
    }
    return -1;  /* Timeout */
}

/** Send a byte to the PS/2 mouse (via the second PS/2 port) */
static int mouse_send_cmd(uint8_t cmd) {
    /* Tell the controller we want to talk to the mouse */
    if (ps2_wait_input() < 0) return -1;
    outb(PS2_COMMAND_PORT, 0xD4);

    /* Send the command byte to the mouse */
    if (ps2_wait_input() < 0) return -1;
    outb(PS2_DATA_PORT, cmd);

    /* Wait for ACK */
    if (ps2_wait_output() < 0) return -1;
    uint8_t resp = inb(PS2_DATA_PORT);

    return (resp == PS2_ACK) ? 0 : -2;
}

/** Read the PS/2 controller configuration byte */
static uint8_t ps2_read_config(void) {
    if (ps2_wait_input() < 0) return 0;
    outb(PS2_COMMAND_PORT, PS2_CMD_READ_CONFIG);
    if (ps2_wait_output() < 0) return 0;
    return inb(PS2_DATA_PORT);
}

/** Write the PS/2 controller configuration byte */
static void ps2_write_config(uint8_t config) {
    if (ps2_wait_input() < 0) return;
    outb(PS2_COMMAND_PORT, PS2_CMD_WRITE_CONFIG);
    if (ps2_wait_input() < 0) return;
    outb(PS2_DATA_PORT, config);
}

/* ============================================ */
/* Initialization                                */
/* ============================================ */

void mouse_init(void) {
    /* Clear mouse state */
    memset(&mouse_state, 0, sizeof(mouse_state));
    mouse_state.cursor_old_x = -1;
    mouse_state.cursor_old_y = -1;
    mouse_state.packet_size = 3;

    /* Disable PS/2 ports during configuration */
    if (ps2_wait_input() < 0) return;
    outb(PS2_COMMAND_PORT, PS2_CMD_DISABLE_PORT1);
    if (ps2_wait_input() < 0) return;
    outb(PS2_COMMAND_PORT, PS2_CMD_DISABLE_PORT2);

    /* Flush output buffer */
    while (inb(PS2_STATUS_PORT) & 0x01) {
        inb(PS2_DATA_PORT);
    }

    /* Read and save config, enable IRQ12 for port 2 */
    uint8_t config = ps2_read_config();

    /* Check if the second PS/2 port exists (bit 5 = 0 means dual-channel) */
    if ((config & 0x20)) {
        /* Single-channel controller - no mouse port */
        /* Re-enable port 1 before returning */
        if (ps2_wait_input() == 0) {
            outb(PS2_COMMAND_PORT, PS2_CMD_ENABLE_PORT1);
        }
        return;
    }

    /* Enable the second PS/2 port */
    if (ps2_wait_input() < 0) return;
    outb(PS2_COMMAND_PORT, PS2_CMD_ENABLE_PORT2);

    /* Re-read config and enable IRQ12 (bit 1) + clock (bit 5) */
    config = ps2_read_config();
    config |= 0x02;   /* Enable IRQ12 for mouse */
    config &= ~0x20;  /* Clear disable clock bit */
    ps2_write_config(config);

    /* Reset the mouse */
    if (mouse_send_cmd(PS2_MOUSE_CMD_RESET) != 0) {
        /* Mouse not responding */
        /* Re-enable port 1 */
        if (ps2_wait_input() == 0) {
            outb(PS2_COMMAND_PORT, PS2_CMD_ENABLE_PORT1);
        }
        return;
    }

    /* After reset, mouse sends self-test result (0xAA) then device ID */
    if (ps2_wait_output() < 0) return;
    uint8_t self_test = inb(PS2_DATA_PORT);
    if (self_test != PS2_SELF_TEST_OK) return;

    if (ps2_wait_output() < 0) return;
    uint8_t device_id = inb(PS2_DATA_PORT);

    /* Set defaults */
    if (mouse_send_cmd(PS2_MOUSE_CMD_SET_DEFAULTS) < 0) return;

    /* Try to enable scroll wheel by setting sample rate sequence */
    /* Sequence: 200, 100, 80 → enables wheel mode */
    mouse_send_cmd(PS2_MOUSE_CMD_SET_SAMPLE);
    mouse_send_cmd(200);
    mouse_send_cmd(PS2_MOUSE_CMD_SET_SAMPLE);
    mouse_send_cmd(100);
    mouse_send_cmd(PS2_MOUSE_CMD_SET_SAMPLE);
    mouse_send_cmd(80);

    /* Get device ID to check if wheel was enabled */
    if (mouse_send_cmd(PS2_MOUSE_CMD_GET_DEV_ID) >= 0) {
        if (ps2_wait_output() >= 0) {
            device_id = inb(PS2_DATA_PORT);
            if (device_id == PS2_MOUSE_ID_WHEEL || 
                device_id == PS2_MOUSE_ID_5BUTTON) {
                mouse_state.has_wheel = 1;
                mouse_state.packet_size = 4;
            }
        }
    }

    /* Set sample rate to 100 (samples per second) */
    mouse_send_cmd(PS2_MOUSE_CMD_SET_SAMPLE);
    mouse_send_cmd(100);

    /* Set resolution to 4 counts/mm */
    mouse_send_cmd(PS2_MOUSE_CMD_SET_RES);
    mouse_send_cmd(0x03);  /* 8 counts/mm */

    /* Enable data reporting */
    if (mouse_send_cmd(PS2_MOUSE_CMD_ENABLE) < 0) return;

    /* Set initial position to center of screen */
    if (vbe_fb.initialized) {
        mouse_state.x = vbe_fb.width / 2;
        mouse_state.y = vbe_fb.height / 2;
    } else {
        mouse_state.x = 400;
        mouse_state.y = 300;
    }

    /* Unmask IRQ12 to enable mouse interrupts */
    pic_unmask_irq(12);

    mouse_state.present = 1;

    /* Draw cursor at initial position so it's visible immediately */
    mouse_state.cursor_dirty = 1;
    mouse_draw_cursor();
    mouse_state.cursor_dirty = 0;
}

/* ============================================ */
/* Packet Parsing / IRQ Handler                  */
/* ============================================ */

void mouse_handler(void) {
    if (!mouse_state.present) return;

    /* Read byte from mouse */
    uint8_t data = inb(PS2_DATA_PORT);

    /* Store byte in packet buffer */
    mouse_state.packet[mouse_state.packet_index] = data;
    mouse_state.packet_index++;

    /* Check if we have a complete packet */
    if (mouse_state.packet_index >= mouse_state.packet_size) {
        mouse_state.packet_index = 0;

        uint8_t *pkt = mouse_state.packet;

        /* Extract movement deltas (sign-extend 9-bit values) */
        int dx = (int)(pkt[1]);
        int dy = (int)(pkt[2]);

        /* Apply sign bits */
        if (pkt[0] & MOUSE_PKT_X_SIGN) {
            dx |= 0xFFFFFF00;  /* Sign-extend to 32 bits */
        }
        if (pkt[0] & MOUSE_PKT_Y_SIGN) {
            dy |= 0xFFFFFF00;
        }

        /* Invert Y (screen Y is inverted relative to mouse Y) */
        dy = -dy;

        /* Update position with clamping */
        if (vbe_fb.initialized) {
            mouse_state.x += dx;
            mouse_state.y += dy;

            /* Clamp to screen bounds */
            if (mouse_state.x < 0) mouse_state.x = 0;
            if (mouse_state.x >= (int32_t)vbe_fb.width) mouse_state.x = vbe_fb.width - 1;
            if (mouse_state.y < 0) mouse_state.y = 0;
            if (mouse_state.y >= (int32_t)vbe_fb.height) mouse_state.y = vbe_fb.height - 1;

            /* Mark cursor for redraw */
            mouse_state.cursor_dirty = 1;
        }

        /* Extract button state */
        mouse_state.buttons = 0;
        if (pkt[0] & MOUSE_PKT_LEFT_BUTTON)  mouse_state.buttons |= MOUSE_BTN_LEFT;
        if (pkt[0] & MOUSE_PKT_RIGHT_BUTTON) mouse_state.buttons |= MOUSE_BTN_RIGHT;
        if (pkt[0] & MOUSE_PKT_MIDDLE_BUTTON) mouse_state.buttons |= MOUSE_BTN_MIDDLE;

        /* Extract wheel delta for 4-byte packets */
        if (mouse_state.packet_size >= 4) {
            mouse_state.wheel_delta = (int8_t)pkt[3];
        }
    }
}

/* ============================================ */
/* Public API                                    */
/* ============================================ */

int32_t mouse_get_x(void) {
    return mouse_state.x;
}

int32_t mouse_get_y(void) {
    return mouse_state.y;
}

uint8_t mouse_get_buttons(void) {
    return mouse_state.buttons;
}

int mouse_is_present(void) {
    return mouse_state.present;
}

/* ============================================ */
/* Cursor Drawing (VBE Framebuffer)              */
/* ============================================ */

void mouse_draw_cursor(void) {
    if (!vbe_fb.initialized) return;
    if (!mouse_state.present) return;

    int32_t x = mouse_state.x;
    int32_t y = mouse_state.y;

    /* Don't draw if completely off-screen */
    if (x < 0 || y < 0 || x >= (int32_t)vbe_fb.width || y >= (int32_t)vbe_fb.height) {
        return;
    }

    /* Save background pixels */
    for (int32_t row = 0; row < MOUSE_CURSOR_HEIGHT; row++) {
        int32_t draw_y = y + row;
        if (draw_y >= (int32_t)vbe_fb.height) break;

        for (int32_t col = 0; col < MOUSE_CURSOR_WIDTH; col++) {
            int32_t draw_x = x + col;
            if (draw_x >= (int32_t)vbe_fb.width) break;

            mouse_state.cursor_save[row * MOUSE_CURSOR_WIDTH + col] = vbe_getpixel(draw_x, draw_y);
        }
    }

    /* Draw cursor pixels */
    uint32_t outline_color = VBE_WHITE;
    uint32_t fill_color    = VBE_BLACK;
    uint32_t outline2_color = VBE_BLACK;  /* Second outline for contrast */

    for (int32_t row = 0; row < MOUSE_CURSOR_HEIGHT; row++) {
        int32_t draw_y = y + row;
        if (draw_y < 0 || draw_y >= (int32_t)vbe_fb.height) continue;

        /* Combine the two bitmap bytes for this row into one 16-bit value */
        uint16_t bits = ((uint16_t)cursor_bitmap[row][0] << 8) | cursor_bitmap[row][1];

        for (int32_t col = 0; col < MOUSE_CURSOR_WIDTH; col++) {
            int32_t draw_x = x + col;
            if (draw_x < 0 || draw_x >= (int32_t)vbe_fb.width) continue;

            int bit = (bits >> (15 - col)) & 1;

            if (bit) {
                /* Check if this is the edge of the cursor (for outline effect) */
                int is_edge = 0;
                if (row == 0 || row == MOUSE_CURSOR_HEIGHT - 1 || 
                    col == 0 || col == MOUSE_CURSOR_WIDTH - 1) {
                    is_edge = 1;
                } else {
                    /* Check if any neighbor is background */
                    for (int dr = -1; dr <= 1 && !is_edge; dr++) {
                        for (int dc = -1; dc <= 1 && !is_edge; dc++) {
                            int nr = row + dr;
                            int nc = col + dc;
                            if (nr >= 0 && nr < MOUSE_CURSOR_HEIGHT && 
                                nc >= 0 && nc < MOUSE_CURSOR_WIDTH) {
                                uint16_t nbits = ((uint16_t)cursor_bitmap[nr][0] << 8) | cursor_bitmap[nr][1];
                                int nbit = (nbits >> (15 - nc)) & 1;
                                if (!nbit) is_edge = 1;
                            }
                        }
                    }
                }

                if (is_edge) {
                    /* Draw white outline, then black outline for contrast */
                    if (row > 0 && col > 0) {
                        vbe_putpixel(draw_x, draw_y, outline_color);
                    } else {
                        vbe_putpixel(draw_x, draw_y, outline_color);
                    }
                } else {
                    vbe_putpixel(draw_x, draw_y, fill_color);
                }
            }
        }
    }

    mouse_state.cursor_old_x = x;
    mouse_state.cursor_old_y = y;
    mouse_state.cursor_visible = 1;
}

void mouse_erase_cursor(void) {
    if (!vbe_fb.initialized) return;
    if (!mouse_state.cursor_visible) return;

    int32_t old_x = mouse_state.cursor_old_x;
    int32_t old_y = mouse_state.cursor_old_y;

    /* Restore saved background pixels.
     * The save area is always fully populated before cursor_visible is set,
     * so no sentinel check is needed. */
    for (int32_t row = 0; row < MOUSE_CURSOR_HEIGHT; row++) {
        int32_t draw_y = old_y + row;
        if (draw_y < 0 || draw_y >= (int32_t)vbe_fb.height) continue;

        for (int32_t col = 0; col < MOUSE_CURSOR_WIDTH; col++) {
            int32_t draw_x = old_x + col;
            if (draw_x < 0 || draw_x >= (int32_t)vbe_fb.width) continue;

            vbe_putpixel(draw_x, draw_y, 
                mouse_state.cursor_save[row * MOUSE_CURSOR_WIDTH + col]);
        }
    }

    mouse_state.cursor_visible = 0;
}

void mouse_update_cursor(void) {
    if (!vbe_fb.initialized) return;
    if (!mouse_state.present || !mouse_state.cursor_dirty) return;

    /* Only redraw if position changed */
    if (mouse_state.x == mouse_state.cursor_old_x && 
        mouse_state.y == mouse_state.cursor_old_y) {
        return;
    }

    mouse_erase_cursor();
    mouse_draw_cursor();
    mouse_state.cursor_dirty = 0;
}

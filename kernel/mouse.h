/**
 * Bengal Tiger OS - PS/2 Mouse Driver
 *
 * Full PS/2 mouse driver with:
 * - IRQ12-based interrupt handling
 * - Standard 3-byte and 4-byte (wheel) packet parsing
 * - Cursor position tracking with screen bounds clamping
 * - Left/Middle/Right button state
 * - VBE framebuffer cursor rendering
 *
 * @file mouse.h
 * @author Bengal Tiger OS (BengalEmpire)
 * @version 0.5.0
 */

#ifndef MOUSE_H
#define MOUSE_H

#include "common.h"

/* PS/2 Controller I/O Ports */
#define PS2_DATA_PORT       0x60
#define PS2_STATUS_PORT     0x64
#define PS2_COMMAND_PORT    0x64

/* PS/2 Controller Commands */
#define PS2_CMD_READ_CONFIG     0x20
#define PS2_CMD_WRITE_CONFIG    0x60
#define PS2_CMD_DISABLE_PORT2   0xA7
#define PS2_CMD_ENABLE_PORT2    0xA8
#define PS2_CMD_TEST_PORT2      0xA9
#define PS2_CMD_SELF_TEST       0xAA
#define PS2_CMD_TEST_PORT1      0xAB
#define PS2_CMD_DISABLE_PORT1   0xAD
#define PS2_CMD_ENABLE_PORT1    0xAE

/* PS/2 Mouse Device Commands */
#define PS2_MOUSE_CMD_RESET         0xFF
#define PS2_MOUSE_CMD_RESEND        0xFE
#define PS2_MOUSE_CMD_SET_DEFAULTS  0xF6
#define PS2_MOUSE_CMD_DISABLE       0xF5
#define PS2_MOUSE_CMD_ENABLE        0xF4
#define PS2_MOUSE_CMD_SET_SAMPLE    0xF3
#define PS2_MOUSE_CMD_GET_DEV_ID    0xF2
#define PS2_MOUSE_CMD_SET_REMOTE    0xF0
#define PS2_MOUSE_CMD_SET_WRAP      0xEE
#define PS2_MOUSE_CMD_SET_RESET_WRAP 0xEC
#define PS2_MOUSE_CMD_READ_DATA     0xEB
#define PS2_MOUSE_CMD_SET_STREAM    0xEA
#define PS2_MOUSE_CMD_STATUS_REQ    0xE9
#define PS2_MOUSE_CMD_SET_RES       0xE8
#define PS2_MOUSE_CMD_SET_SCALE21   0xE7
#define PS2_MOUSE_CMD_SET_SCALE11   0xE6

/* PS/2 Responses */
#define PS2_ACK             0xFA
#define PS2_SELF_TEST_OK    0x55
#define PS2_MOUSE_ID_STANDARD   0x00
#define PS2_MOUSE_ID_WHEEL     0x03
#define PS2_MOUSE_ID_5BUTTON   0x04

/* Mouse packet byte 0 bit flags */
#define MOUSE_PKT_LEFT_BUTTON   0x01
#define MOUSE_PKT_RIGHT_BUTTON  0x02
#define MOUSE_PKT_MIDDLE_BUTTON 0x04
#define MOUSE_PKT_X_SIGN        0x10  /* X movement is negative */
#define MOUSE_PKT_Y_SIGN        0x20  /* Y movement is negative */
#define MOUSE_PKT_X_OVERFLOW    0x40
#define MOUSE_PKT_Y_OVERFLOW    0x80

/* Cursor size */
#define MOUSE_CURSOR_WIDTH  12
#define MOUSE_CURSOR_HEIGHT 19

/** Mouse button state flags */
#define MOUSE_BTN_LEFT   1
#define MOUSE_BTN_RIGHT  2
#define MOUSE_BTN_MIDDLE 4

/** Mouse state structure */
typedef struct {
    int32_t x;                  /* Current X position (0 to screen_width) */
    int32_t y;                  /* Current Y position (0 to screen_height) */
    uint8_t buttons;            /* Button state (MOUSE_BTN_* flags) */
    int8_t  wheel_delta;        /* Scroll wheel delta since last read */
    uint8_t present;            /* 1 if mouse detected and initialized */
    uint8_t has_wheel;          /* 1 if mouse supports scroll wheel */
    /* Packet state machine */
    uint8_t  packet_index;      /* Current byte position in packet */
    uint8_t  packet[4];         /* Accumulated packet bytes */
    uint8_t  packet_size;       /* Bytes per packet (3 or 4) */
    /* Cursor background save area */
    uint32_t cursor_save[MOUSE_CURSOR_WIDTH * MOUSE_CURSOR_HEIGHT];
    int32_t  cursor_old_x;      /* Previous cursor X for restore */
    int32_t  cursor_old_y;      /* Previous cursor Y for restore */
    uint8_t  cursor_dirty;      /* 1 if cursor needs redraw */
    uint8_t  cursor_visible;    /* 1 if cursor is currently shown */
} mouse_state_t;

/** Global mouse state */
extern mouse_state_t mouse_state;

/* ============================================ */
/* Mouse Driver API                             */
/* ============================================ */

/**
 * Initialize the PS/2 mouse driver.
 * Detects mouse presence, enables IRQ12, sets up packet streaming.
 * Must be called after IDT and PIC are initialized, before STI.
 */
void mouse_init(void);

/**
 * Mouse interrupt handler - called by IRQ12.
 * Reads bytes from the PS/2 data port, assembles packets,
 * updates position and button state.
 */
void mouse_handler(void);

/**
 * Get current mouse position X.
 * @return X coordinate (0 to screen width)
 */
int32_t mouse_get_x(void);

/**
 * Get current mouse position Y.
 * @return Y coordinate (0 to screen height)
 */
int32_t mouse_get_y(void);

/**
 * Get mouse button state.
 * @return Bitmask of MOUSE_BTN_LEFT | MOUSE_BTN_RIGHT | MOUSE_BTN_MIDDLE
 */
uint8_t mouse_get_buttons(void);

/**
 * Check if a mouse is present and initialized.
 * @return 1 if present, 0 otherwise
 */
int mouse_is_present(void);

/**
 * Draw the mouse cursor on the VBE framebuffer.
 * Saves background pixels before drawing the cursor bitmap.
 */
void mouse_draw_cursor(void);

/**
 * Erase the mouse cursor from the VBE framebuffer.
 * Restores saved background pixels.
 */
void mouse_erase_cursor(void);

/**
 * Update cursor position and redraw on the framebuffer.
 * Erases old cursor, updates position, draws new cursor.
 * Only works when VBE framebuffer is active.
 */
void mouse_update_cursor(void);

#endif /* MOUSE_H */

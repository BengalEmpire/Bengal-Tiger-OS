/**
 * Bengal Tiger OS - Enhanced Keyboard Driver
 * 
 * Full PS/2 keyboard driver with:
 * - Shift, Caps Lock, Ctrl, Alt support
 * - Function keys detection
 * - Key repeat handling
 * - Ring buffer for asynchronous input
 * 
 * @file keyboard.h
 * @author Bengal Tiger OS (BengalEmpire)
 * @version 0.3.0
 */

#ifndef KEYBOARD_H
#define KEYBOARD_H

#include "common.h"

/* Keyboard I/O Ports */
#define KB_DATA_PORT        0x60
#define KB_STATUS_PORT      0x64
#define KB_COMMAND_PORT     0x64

/* Special Key Scancodes (Set 1) */
#define SC_ESCAPE           0x01
#define SC_BACKSPACE        0x0E
#define SC_TAB              0x0F
#define SC_ENTER            0x1C
#define SC_LEFT_CTRL        0x1D
#define SC_LEFT_SHIFT       0x2A
#define SC_RIGHT_SHIFT      0x36
#define SC_LEFT_ALT         0x38
#define SC_CAPS_LOCK        0x3A
#define SC_F1               0x3B
#define SC_F2               0x3C
#define SC_F3               0x3D
#define SC_F4               0x3E
#define SC_F5               0x3F
#define SC_F6               0x40
#define SC_F7               0x41
#define SC_F8               0x42
#define SC_F9               0x43
#define SC_F10              0x44
#define SC_F11              0x57
#define SC_F12              0x58
#define SC_NUM_LOCK         0x45
#define SC_SCROLL_LOCK      0x46
#define SC_HOME             0x47
#define SC_UP               0x48
#define SC_PAGE_UP          0x49
#define SC_LEFT             0x4B
#define SC_RIGHT            0x4D
#define SC_END              0x4F
#define SC_DOWN             0x50
#define SC_PAGE_DOWN        0x51
#define SC_INSERT           0x52
#define SC_DELETE           0x53

/* Key Release Bit */
#define SC_RELEASE_BIT      0x80

/* Ring Buffer Size (must be power of 2) */
#define KB_BUFFER_SIZE      64

/* Modifier Key Flags */
typedef struct {
    uint8_t shift    : 1;
    uint8_t ctrl     : 1;
    uint8_t alt      : 1;
    uint8_t caps     : 1;
    uint8_t num      : 1;
    uint8_t scroll   : 1;
    uint8_t reserved : 2;
} keyboard_modifiers_t;

/* Extended Key Code (includes modifiers) */
typedef struct {
    char ascii;             /* ASCII character (0 if special key) */
    uint8_t scancode;       /* Raw scancode */
    keyboard_modifiers_t mods;  /* Modifier state at key press */
} keycode_t;

/* Global Modifier State */
extern keyboard_modifiers_t kb_modifiers;

/**
 * Initialize the keyboard driver.
 * Sets up IRQ1 handler and clears the keyboard buffer.
 */
void keyboard_init(void);

/**
 * Keyboard install - alias for keyboard_init.
 */
void keyboard_install(void);

/**
 * Keyboard interrupt handler - called by IRQ1.
 */
void keyboard_handler(void);

/**
 * Check if a key is available in the buffer.
 * @return 1 if key available, 0 otherwise
 */
int keyboard_has_key(void);

/**
 * Get next key from buffer (blocking).
 * Waits until a key is available.
 * @return ASCII character (or 0 for special keys)
 */
char keyboard_getchar(void);

/**
 * Get next key from buffer (non-blocking).
 * @return ASCII character, or 0 if no key available
 */
char keyboard_getchar_nonblock(void);

/**
 * Get full keycode from buffer (includes modifiers).
 * @param key Pointer to keycode structure to fill
 * @return 1 if key retrieved, 0 if buffer empty
 */
int keyboard_get_keycode(keycode_t *key);

/**
 * Check if a specific modifier is active.
 */
int keyboard_is_shift_pressed(void);
int keyboard_is_ctrl_pressed(void);
int keyboard_is_alt_pressed(void);
int keyboard_is_caps_on(void);

/**
 * Convert scancode to ASCII based on current modifiers.
 * @param scancode The scancode to convert
 * @return ASCII character, or 0 if no mapping
 */
char scancode_to_ascii(uint8_t scancode);

#endif /* KEYBOARD_H */
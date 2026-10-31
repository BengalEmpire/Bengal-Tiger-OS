/**
 * Bengal Tiger OS - Enhanced PS/2 Keyboard & Unified Input Driver
 *
 * Full Scancode Set 1 + 0xE0 extended scancode support, Ctrl/Shift/Alt/Caps/Num
 * modifiers, and unified non-blocking event queue combining PS/2 keyboard and
 * COM1 serial console ANSI escape sequences.
 *
 * IMPORTANT: Interrupt handlers ONLY enqueue events; command execution runs
 * in normal thread context with interrupts enabled (IF=1).
 *
 * @file keyboard.h
 * @author Bengal Tiger OS (BengalEmpire)
 * @version 1.0.0
 */

#ifndef KEYBOARD_H
#define KEYBOARD_H

#include "common.h"

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
#define SC_F11              0x57
#define SC_F12              0x58

#define SC_RELEASE_BIT      0x80
#define KB_BUFFER_SIZE      256

/* High-level input event codes (> 255 for special navigation keys) */
#define KEY_NONE            0
#define KEY_SPECIAL_UP      1001
#define KEY_SPECIAL_DOWN    1002
#define KEY_SPECIAL_LEFT    1003
#define KEY_SPECIAL_RIGHT   1004
#define KEY_SPECIAL_HOME    1005
#define KEY_SPECIAL_END     1006
#define KEY_SPECIAL_DELETE  1007
#define KEY_SPECIAL_PGUP    1008
#define KEY_SPECIAL_PGDN    1009
#define KEY_SPECIAL_F1      1010
#define KEY_SPECIAL_F2      1011

/* Ctrl+Key ASCII control codes */
#define CTRL_KEY(c)         ((c) & 0x1F)

typedef struct {
    uint8_t shift    : 1;
    uint8_t ctrl     : 1;
    uint8_t alt      : 1;
    uint8_t caps     : 1;
    uint8_t num      : 1;
    uint8_t scroll   : 1;
    uint8_t reserved : 2;
} keyboard_modifiers_t;

typedef struct {
    char ascii;
    uint8_t scancode;
    keyboard_modifiers_t mods;
} keycode_t;

extern keyboard_modifiers_t kb_modifiers;

void keyboard_init(void);
void keyboard_install(void);
void keyboard_handler(void);

int  keyboard_has_key(void);
char keyboard_getchar(void);
char keyboard_getchar_nonblock(void);
int  keyboard_get_keycode(keycode_t *key);

/* Unified event API (PS/2 + Serial COM1 ANSI sequences) */
int  input_poll_event(void);
int  input_wait_event(void);

int keyboard_is_shift_pressed(void);
int keyboard_is_ctrl_pressed(void);
int keyboard_is_alt_pressed(void);
int keyboard_is_caps_on(void);

char scancode_to_ascii(uint8_t scancode);

#endif /* KEYBOARD_H */

/**
 * Bengal Tiger OS - Enhanced Keyboard Driver Implementation
 * 
 * @file keyboard.c
 * @author Bengal Tiger OS (BengalEmpire)
 * @version 0.3.0
 */

#include "keyboard.h"
#include "common.h"
#include "shell.h"

/* Global modifier state */
keyboard_modifiers_t kb_modifiers = {0, 0, 0, 0, 0, 0, 0};

/* Ring buffer for async key input */
static char key_buffer[KB_BUFFER_SIZE];
static volatile uint8_t buffer_head = 0;
static volatile uint8_t buffer_tail = 0;

/* Extended key buffer */
static keycode_t keycode_buffer[KB_BUFFER_SIZE];
static volatile uint8_t kc_buffer_head = 0;
static volatile uint8_t kc_buffer_tail = 0;

/* Lowercase scancode to ASCII mapping (US QWERTY) */
static const char scancode_lower[128] = {
    0, 27, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', '\b',  /* 0x00 - 0x0E */
    '\t', 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n',    /* 0x0F - 0x1C */
    0, 'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`',            /* 0x1D - 0x29 */
    0, '\\', 'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/', 0, '*',         /* 0x2A - 0x37 */
    0, ' ', 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,                             /* 0x38 - 0x46 */
    '7', '8', '9', '-', '4', '5', '6', '+', '1', '2', '3', '0', '.',           /* 0x47 - 0x53 (numpad) */
    0, 0, 0, 0, 0,                                                             /* 0x54 - 0x58 */
    /* Rest are zeros */
};

/* Uppercase/shifted scancode to ASCII mapping */
static const char scancode_upper[128] = {
    0, 27, '!', '@', '#', '$', '%', '^', '&', '*', '(', ')', '_', '+', '\b',
    '\t', 'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '{', '}', '\n',
    0, 'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L', ':', '"', '~',
    0, '|', 'Z', 'X', 'C', 'V', 'B', 'N', 'M', '<', '>', '?', 0, '*',
    0, ' ', 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    '7', '8', '9', '-', '4', '5', '6', '+', '1', '2', '3', '0', '.',
    0, 0, 0, 0, 0,
};

void keyboard_init(void) {
    /* Clear buffers */
    buffer_head = buffer_tail = 0;
    kc_buffer_head = kc_buffer_tail = 0;
    
    /* Reset modifiers */
    kb_modifiers.shift = 0;
    kb_modifiers.ctrl = 0;
    kb_modifiers.alt = 0;
    kb_modifiers.caps = 0;
    kb_modifiers.num = 0;
    kb_modifiers.scroll = 0;
    
    /* Clear keyboard buffer by reading any pending data */
    while (inb(KB_STATUS_PORT) & 0x01) {
        inb(KB_DATA_PORT);
    }
}

void keyboard_install(void) {
    keyboard_init();
}

char scancode_to_ascii(uint8_t scancode) {
    if (scancode >= 128) return 0;
    
    int use_upper = 0;
    
    /* Determine case based on shift and caps lock */
    if (kb_modifiers.shift) {
        use_upper = !kb_modifiers.caps;
    } else {
        use_upper = kb_modifiers.caps;
    }
    
    /* For non-letter keys, only shift matters */
    char lower = scancode_lower[scancode];
    char upper = scancode_upper[scancode];
    
    /* Check if it's a letter (affected by caps lock) */
    if (lower >= 'a' && lower <= 'z') {
        return use_upper ? upper : lower;
    }
    
    /* Non-letters: shift only */
    return kb_modifiers.shift ? upper : lower;
}

static void buffer_push(char ch) {
    uint8_t next_head = (buffer_head + 1) & (KB_BUFFER_SIZE - 1);
    if (next_head != buffer_tail) {
        key_buffer[buffer_head] = ch;
        buffer_head = next_head;
    }
    /* Buffer full - drop the key */
}

static void keycode_push(keycode_t *key) {
    uint8_t next_head = (kc_buffer_head + 1) & (KB_BUFFER_SIZE - 1);
    if (next_head != kc_buffer_tail) {
        keycode_buffer[kc_buffer_head] = *key;
        kc_buffer_head = next_head;
    }
}

void keyboard_handler(void) {
    uint8_t scancode = inb(KB_DATA_PORT);
    uint8_t released = scancode & SC_RELEASE_BIT;
    uint8_t key = scancode & ~SC_RELEASE_BIT;
    
    /* Handle modifier keys */
    switch (key) {
        case SC_LEFT_SHIFT:
        case SC_RIGHT_SHIFT:
            kb_modifiers.shift = !released;
            return;
            
        case SC_LEFT_CTRL:
            kb_modifiers.ctrl = !released;
            return;
            
        case SC_LEFT_ALT:
            kb_modifiers.alt = !released;
            return;
            
        case SC_CAPS_LOCK:
            if (!released) {
                kb_modifiers.caps = !kb_modifiers.caps;
            }
            return;
            
        case SC_NUM_LOCK:
            if (!released) {
                kb_modifiers.num = !kb_modifiers.num;
            }
            return;
            
        case SC_SCROLL_LOCK:
            if (!released) {
                kb_modifiers.scroll = !kb_modifiers.scroll;
            }
            return;
    }
    
    /* Ignore key releases for regular keys */
    if (released) return;
    
    /* Handle special keys */
    char ascii = 0;
    switch (key) {
        case SC_UP:
            /* Send escape sequence for up arrow */
            shell_handler_special(SPECIAL_KEY_UP);
            return;
        case SC_DOWN:
            shell_handler_special(SPECIAL_KEY_DOWN);
            return;
        case SC_LEFT:
            shell_handler_special(SPECIAL_KEY_LEFT);
            return;
        case SC_RIGHT:
            shell_handler_special(SPECIAL_KEY_RIGHT);
            return;
        case SC_HOME:
            shell_handler_special(SPECIAL_KEY_HOME);
            return;
        case SC_END:
            shell_handler_special(SPECIAL_KEY_END);
            return;
        case SC_DELETE:
            shell_handler_special(SPECIAL_KEY_DELETE);
            return;
        case SC_PAGE_UP:
        case SC_PAGE_DOWN:
            /* TODO: Implement page scroll */
            return;
        case SC_F1:
        case SC_F2:
        case SC_F3:
        case SC_F4:
        case SC_F5:
        case SC_F6:
        case SC_F7:
        case SC_F8:
        case SC_F9:
        case SC_F10:
        case SC_F11:
        case SC_F12:
            /* TODO: Handle function keys */
            return;
    }
    
    /* Convert normal key to ASCII */
    ascii = scancode_to_ascii(key);
    
    if (ascii) {
        /* Push to buffer */
        buffer_push(ascii);
        
        /* Create extended keycode */
        keycode_t kc;
        kc.ascii = ascii;
        kc.scancode = key;
        kc.mods = kb_modifiers;
        keycode_push(&kc);
        
        /* Send to shell handler */
        shell_handler(ascii);
    }
}

int keyboard_has_key(void) {
    return buffer_head != buffer_tail;
}

char keyboard_getchar(void) {
    /* Wait for key */
    while (buffer_head == buffer_tail) {
        __asm__ volatile("hlt");
    }
    
    char ch = key_buffer[buffer_tail];
    buffer_tail = (buffer_tail + 1) & (KB_BUFFER_SIZE - 1);
    return ch;
}

char keyboard_getchar_nonblock(void) {
    if (buffer_head == buffer_tail) {
        return 0;
    }
    
    char ch = key_buffer[buffer_tail];
    buffer_tail = (buffer_tail + 1) & (KB_BUFFER_SIZE - 1);
    return ch;
}

int keyboard_get_keycode(keycode_t *key) {
    if (kc_buffer_head == kc_buffer_tail) {
        return 0;
    }
    
    *key = keycode_buffer[kc_buffer_tail];
    kc_buffer_tail = (kc_buffer_tail + 1) & (KB_BUFFER_SIZE - 1);
    return 1;
}

int keyboard_is_shift_pressed(void) {
    return kb_modifiers.shift;
}

int keyboard_is_ctrl_pressed(void) {
    return kb_modifiers.ctrl;
}

int keyboard_is_alt_pressed(void) {
    return kb_modifiers.alt;
}

int keyboard_is_caps_on(void) {
    return kb_modifiers.caps;
}
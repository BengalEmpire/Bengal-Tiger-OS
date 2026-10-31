/**
 * Bengal Tiger OS - Enhanced PS/2 Keyboard & Unified Input Implementation
 *
 * @file keyboard.c
 * @author Bengal Tiger OS (BengalEmpire)
 * @version 1.0.0
 */

#include "keyboard.h"
#include "serial.h"
#include "mouse.h"
#include "common.h"

keyboard_modifiers_t kb_modifiers = {0, 0, 0, 0, 1, 0, 0};

/* ASCII ring buffer */
static char key_buffer[KB_BUFFER_SIZE];
static volatile uint16_t buffer_head = 0;
static volatile uint16_t buffer_tail = 0;

/* Extended keycode ring buffer */
static keycode_t keycode_buffer[KB_BUFFER_SIZE];
static volatile uint16_t kc_buffer_head = 0;
static volatile uint16_t kc_buffer_tail = 0;

/* Unified event ring buffer (stores ASCII 1..127 or KEY_SPECIAL_* > 1000) */
static int event_buffer[KB_BUFFER_SIZE];
static volatile uint16_t ev_head = 0;
static volatile uint16_t ev_tail = 0;

/* Track 0xE0 extended scancode prefix */
static uint8_t e0_prefix = 0;

/* Serial ANSI escape parser state */
static uint8_t ansi_state = 0;
static char    ansi_param = 0;

static const char scancode_lower[128] = {
    0, 27, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', '\b',
    '\t', 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n',
    0, 'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`',
    0, '\\', 'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/', 0, '*',
    0, ' ', 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    '7', '8', '9', '-', '4', '5', '6', '+', '1', '2', '3', '0', '.',
    0, 0, 0, 0, 0,
};

static const char scancode_upper[128] = {
    0, 27, '!', '@', '#', '$', '%', '^', '&', '*', '(', ')', '_', '+', '\b',
    '\t', 'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '{', '}', '\n',
    0, 'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L', ':', '"', '~',
    0, '|', 'Z', 'X', 'C', 'V', 'B', 'N', 'M', '<', '>', '?', 0, '*',
    0, ' ', 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    '7', '8', '9', '-', '4', '5', '6', '+', '1', '2', '3', '0', '.',
    0, 0, 0, 0, 0,
};

static void event_push(int ev) {
    uint16_t next = (uint16_t)((ev_head + 1) & (KB_BUFFER_SIZE - 1));
    if (next != ev_tail) {
        event_buffer[ev_head] = ev;
        ev_head = next;
    }
}

static void buffer_push(char ch) {
    uint16_t next_head = (uint16_t)((buffer_head + 1) & (KB_BUFFER_SIZE - 1));
    if (next_head != buffer_tail) {
        key_buffer[buffer_head] = ch;
        buffer_head = next_head;
    }
}

static void keycode_push(keycode_t *key) {
    uint16_t next_head = (uint16_t)((kc_buffer_head + 1) & (KB_BUFFER_SIZE - 1));
    if (next_head != kc_buffer_tail) {
        keycode_buffer[kc_buffer_head] = *key;
        kc_buffer_head = next_head;
    }
}

void keyboard_init(void) {
    buffer_head = buffer_tail = 0;
    kc_buffer_head = kc_buffer_tail = 0;
    ev_head = ev_tail = 0;
    e0_prefix = 0;
    ansi_state = 0;

    kb_modifiers.shift  = 0;
    kb_modifiers.ctrl   = 0;
    kb_modifiers.alt    = 0;
    kb_modifiers.caps   = 0;
    kb_modifiers.num    = 1; /* Default NumLock ON so numpad digits work */
    kb_modifiers.scroll = 0;

    int limit = 64;
    while (limit-- > 0 && (inb(KB_STATUS_PORT) & 0x01)) {
        (void)inb(KB_DATA_PORT);
    }
}

void keyboard_install(void) {
    keyboard_init();
}

char scancode_to_ascii(uint8_t scancode) {
    if (scancode >= 128) return 0;

    int use_upper = kb_modifiers.shift ? !kb_modifiers.caps : kb_modifiers.caps;
    char lower = scancode_lower[scancode];
    char upper = scancode_upper[scancode];

    if (lower >= 'a' && lower <= 'z') {
        if (kb_modifiers.ctrl) {
            return (char)CTRL_KEY(lower);
        }
        return use_upper ? upper : lower;
    }

    return kb_modifiers.shift ? upper : lower;
}

void keyboard_handler(void) {
    uint8_t scancode = inb(KB_DATA_PORT);

    if (scancode == 0xE0) {
        e0_prefix = 1;
        return;
    }

    uint8_t is_ext   = e0_prefix;
    e0_prefix        = 0;
    uint8_t released = scancode & SC_RELEASE_BIT;
    uint8_t key      = scancode & (uint8_t)~SC_RELEASE_BIT;

    /* Modifier keys (both left and 0xE0 right variants) */
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
            if (!released) kb_modifiers.caps = !kb_modifiers.caps;
            return;
        case SC_NUM_LOCK:
            if (!released) kb_modifiers.num = !kb_modifiers.num;
            return;
        case SC_SCROLL_LOCK:
            if (!released) kb_modifiers.scroll = !kb_modifiers.scroll;
            return;
    }

    if (released) return;

    /* Extended 0xE0 keypad/navigation keys OR navigation when NumLock is OFF */
    if (is_ext) {
        switch (key) {
            case SC_ENTER:     buffer_push('\n'); event_push('\n'); return;
            case 0x35:         buffer_push('/');  event_push('/');  return; /* Keypad '/' */
            case SC_UP:        event_push(KEY_SPECIAL_UP);     return;
            case SC_DOWN:      event_push(KEY_SPECIAL_DOWN);   return;
            case SC_LEFT:      event_push(KEY_SPECIAL_LEFT);   return;
            case SC_RIGHT:     event_push(KEY_SPECIAL_RIGHT);  return;
            case SC_HOME:      event_push(KEY_SPECIAL_HOME);   return;
            case SC_END:       event_push(KEY_SPECIAL_END);    return;
            case SC_DELETE:    event_push(KEY_SPECIAL_DELETE); return;
            case SC_PAGE_UP:   event_push(KEY_SPECIAL_PGUP);   return;
            case SC_PAGE_DOWN: event_push(KEY_SPECIAL_PGDN);   return;
            default: return;
        }
    }

    /* Non-extended keypad navigation when NumLock is OFF */
    if (!kb_modifiers.num) {
        switch (key) {
            case SC_UP:        event_push(KEY_SPECIAL_UP);     return;
            case SC_DOWN:      event_push(KEY_SPECIAL_DOWN);   return;
            case SC_LEFT:      event_push(KEY_SPECIAL_LEFT);   return;
            case SC_RIGHT:     event_push(KEY_SPECIAL_RIGHT);  return;
            case SC_HOME:      event_push(KEY_SPECIAL_HOME);   return;
            case SC_END:       event_push(KEY_SPECIAL_END);    return;
            case SC_DELETE:    event_push(KEY_SPECIAL_DELETE); return;
            case SC_PAGE_UP:   event_push(KEY_SPECIAL_PGUP);   return;
            case SC_PAGE_DOWN: event_push(KEY_SPECIAL_PGDN);   return;
            default: break;
        }
    }

    if (key == SC_F1) { event_push(KEY_SPECIAL_F1); return; }
    if (key == SC_F2) { event_push(KEY_SPECIAL_F2); return; }
    if (key >= SC_F3 && key <= SC_F10) return;
    if (key == SC_F11 || key == SC_F12) return;

    char ascii = scancode_to_ascii(key);
    if (ascii) {
        buffer_push(ascii);

        keycode_t kc;
        kc.ascii    = ascii;
        kc.scancode = key;
        kc.mods     = kb_modifiers;
        keycode_push(&kc);

        event_push((unsigned char)ascii);
    }
}

/* Translate incoming serial characters (including VT100/ANSI sequences) into events */
static void pump_serial_events(void) {
    char c;
    while (serial_pop_rx_char(&c)) {
        if (ansi_state == 0) {
            if (c == 27) { /* ESC */
                ansi_state = 1;
                continue;
            }
            if (c == '\r') c = '\n';
            if (c == 0x7F) c = '\b'; /* DEL -> Backspace */
            buffer_push(c);
            event_push((unsigned char)c);
        } else if (ansi_state == 1) {
            if (c == '[' || c == 'O') {
                ansi_state = 2;
                ansi_param = 0;
            } else {
                event_push(27);
                ansi_state = 0;
                if (c == '\r') c = '\n';
                if (c == 0x7F) c = '\b';
                buffer_push(c);
                event_push((unsigned char)c);
            }
        } else if (ansi_state == 2) {
            if (c >= '0' && c <= '9') {
                ansi_param = c;
            } else if (c == '~') {
                if (ansi_param == '1' || ansi_param == '7') event_push(KEY_SPECIAL_HOME);
                else if (ansi_param == '3') event_push(KEY_SPECIAL_DELETE);
                else if (ansi_param == '4' || ansi_param == '8') event_push(KEY_SPECIAL_END);
                else if (ansi_param == '5') event_push(KEY_SPECIAL_PGUP);
                else if (ansi_param == '6') event_push(KEY_SPECIAL_PGDN);
                ansi_state = 0;
            } else {
                if (c == 'A') event_push(KEY_SPECIAL_UP);
                else if (c == 'B') event_push(KEY_SPECIAL_DOWN);
                else if (c == 'C') event_push(KEY_SPECIAL_RIGHT);
                else if (c == 'D') event_push(KEY_SPECIAL_LEFT);
                else if (c == 'H') event_push(KEY_SPECIAL_HOME);
                else if (c == 'F') event_push(KEY_SPECIAL_END);
                ansi_state = 0;
            }
        }
    }
}

int input_poll_event(void) {
    pump_serial_events();

    /* If bare ESC waited with no '[' following, emit ESC */
    if (ansi_state == 1 && !serial_has_data(COM1_PORT)) {
        ansi_state = 0;
        event_push(27);
    }

    uint32_t irq = irq_save();
    if (ev_head == ev_tail) {
        irq_restore(irq);
        return KEY_NONE;
    }
    int ev = event_buffer[ev_tail];
    ev_tail = (uint16_t)((ev_tail + 1) & (KB_BUFFER_SIZE - 1));
    irq_restore(irq);
    return ev;
}

int input_wait_event(void) {
    for (;;) {
        int ev = input_poll_event();
        if (ev != KEY_NONE) return ev;
        mouse_update_cursor();
        if (irq_are_enabled()) {
            __asm__ volatile("hlt" ::: "memory");
        }
    }
}

int keyboard_has_key(void) {
    pump_serial_events();
    return buffer_head != buffer_tail;
}

char keyboard_getchar(void) {
    for (;;) {
        pump_serial_events();
        uint32_t irq = irq_save();
        if (buffer_head != buffer_tail) {
            char ch = key_buffer[buffer_tail];
            buffer_tail = (uint16_t)((buffer_tail + 1) & (KB_BUFFER_SIZE - 1));
            irq_restore(irq);
            return ch;
        }
        irq_restore(irq);
        mouse_update_cursor();
        if (irq_are_enabled()) {
            __asm__ volatile("hlt" ::: "memory");
        }
    }
}

char keyboard_getchar_nonblock(void) {
    pump_serial_events();
    uint32_t irq = irq_save();
    if (buffer_head == buffer_tail) {
        irq_restore(irq);
        return 0;
    }
    char ch = key_buffer[buffer_tail];
    buffer_tail = (uint16_t)((buffer_tail + 1) & (KB_BUFFER_SIZE - 1));
    irq_restore(irq);
    return ch;
}

int keyboard_get_keycode(keycode_t *key) {
    uint32_t irq = irq_save();
    if (kc_buffer_head == kc_buffer_tail) {
        irq_restore(irq);
        return 0;
    }
    *key = keycode_buffer[kc_buffer_tail];
    kc_buffer_tail = (uint16_t)((kc_buffer_tail + 1) & (KB_BUFFER_SIZE - 1));
    irq_restore(irq);
    return 1;
}

int keyboard_is_shift_pressed(void) { return kb_modifiers.shift; }
int keyboard_is_ctrl_pressed(void)  { return kb_modifiers.ctrl;  }
int keyboard_is_alt_pressed(void)   { return kb_modifiers.alt;   }
int keyboard_is_caps_on(void)       { return kb_modifiers.caps;  }

#include <stdint.h>
#include "io.h"
#include "keyboard.h"

static const char scancode_ascii[] = {
    0,      27,     '1',    '2',    '3',    '4',    '5',    '6',    '7',    '8',    '9',    '0',    '-',    '=',    '\b',
    '\t',   'q',    'w',    'e',    'r',    't',    'y',    'u',    'i',    'o',    'p',    '[',    ']',    '\n',
    0,      'a',    's',    'd',    'f',    'g',    'h',    'j',    'k',    'l',    ';',    '\'',   '`',
    0,      '\\',   'z',    'x',    'c',    'v',    'b',    'n',    'm',    ',',    '.',    '/',    0,
    '*',    0,      ' '
};

static const char scancode_ascii_shift[] = {
    0,      27,     '!',    '@',    '#',    '$',    '%',    '^',    '&',    '*',    '(',    ')',    '_',    '+',    '\b',
    '\t',   'Q',    'W',    'E',    'R',    'T',    'Y',    'U',    'I',    'O',    'P',    '{',    '}',    '\n',
    0,      'A',    'S',    'D',    'F',    'G',    'H',    'J',    'K',    'L',    ':',    '"',    '~',
    0,      '|',    'Z',    'X',    'C',    'V',    'B',    'N',    'M',    '<',    '>',    '?',    0,
    '*',    0,      ' '
};

static int shift_pressed = 0;
static int ctrl_pressed = 0;
static int alt_pressed = 0;
static uint8_t last_scancode = 0;
static int extended_scancode = 0;

int keyboard_read_char(void) {
    while (1) {
        if (!(inb(0x64) & 1)) {
            continue;
        }
        
        uint8_t scancode = inb(0x60);
        last_scancode = scancode;

        if (scancode == 0xE0) {
            extended_scancode = 1;
            continue;
        }

        if (extended_scancode) {
            extended_scancode = 0;
            if (scancode & 0x80) continue;
            if (scancode == 0x48) return KEY_ARROW_UP;
            if (scancode == 0x50) return KEY_ARROW_DOWN;
            if (scancode == 0x4B) return KEY_ARROW_LEFT;
            if (scancode == 0x4D) return KEY_ARROW_RIGHT;
            if (scancode == 0x53) return KEY_DELETE;
            continue;
        }
        
        if (scancode & 0x80) {
            uint8_t released = scancode & ~0x80;
            
            if (released == 0x2A || released == 0x36) {
                shift_pressed = 0;
                continue;
            }
            
            if (released == 0x1D) {
                ctrl_pressed = 0;
                continue;
            }
            
            if (released == 0x38) {
                alt_pressed = 0;
                continue;
            }
            
            continue;
        }
        
        if (scancode == 0x2A || scancode == 0x36) {
            shift_pressed = 1;
            continue;
        }
        
        if (scancode == 0x1D) {
            ctrl_pressed = 1;
            continue;
        }
        
        if (scancode == 0x38) {
            alt_pressed = 1;
            continue;
        }
        
        if (ctrl_pressed) {
            if (scancode == 0x1F) return 19;
            if (scancode == 0x2D) return 24;
            if (scancode == 0x1E) return 1;
            if (scancode == 0x30) return 26;
            if (scancode == 0x17) return 9;
            if (scancode == 0x23) return 16;
        }
        
        if (scancode < 60) {
            const char *table = shift_pressed ? scancode_ascii_shift : scancode_ascii;
            char ascii = table[scancode];
            
            if (ascii != 0) {
                return ascii;
            }
        }
    }
}

int keyboard_is_shift_pressed(void) {
    return shift_pressed;
}

int keyboard_is_ctrl_pressed(void) {
    return ctrl_pressed;
}

int keyboard_is_alt_pressed(void) {
    return alt_pressed;
}

uint8_t keyboard_get_last_scancode(void) {
    return last_scancode;
}

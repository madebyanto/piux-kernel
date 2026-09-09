#ifndef KEYBOARD_H
#define KEYBOARD_H

#include <stdint.h>

#define KEY_ARROW_UP    0x100
#define KEY_ARROW_DOWN  0x101
#define KEY_ARROW_LEFT  0x102
#define KEY_ARROW_RIGHT 0x103
#define KEY_DELETE      0x104

int keyboard_read_char(void);
int keyboard_is_shift_pressed(void);
int keyboard_is_ctrl_pressed(void);
int keyboard_is_alt_pressed(void);
uint8_t keyboard_get_last_scancode(void);

#endif

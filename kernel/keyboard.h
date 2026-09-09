#ifndef KEYBOARD_H
#define KEYBOARD_H

#include <stdint.h>

char keyboard_read_char(void);
int keyboard_is_shift_pressed(void);
int keyboard_is_ctrl_pressed(void);
int keyboard_is_alt_pressed(void);
uint8_t keyboard_get_last_scancode(void);

#endif

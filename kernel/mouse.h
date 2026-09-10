#ifndef PIUX_MOUSE_H
#define PIUX_MOUSE_H

#include <stdint.h>

typedef struct {
    int8_t dx;
    int8_t dy;
    int8_t wheel;
    uint8_t buttons;
} mouse_event_t;

void mouse_init(void);
int mouse_poll(mouse_event_t *event);

#endif

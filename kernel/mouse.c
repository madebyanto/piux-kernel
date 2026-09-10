#include <stdint.h>
#include "io.h"
#include "mouse.h"

static int packet_length = 3;
static uint8_t packet[4];
static int packet_index;

static void wait_controller(void) {
    for (uint32_t timeout = 0; timeout < 100000; timeout++) if (!(inb(0x64) & 2)) return;
}

static int read_aux(uint8_t *value) {
    if (!(inb(0x64) & 1) || !(inb(0x64) & 0x20)) return 0;
    *value = inb(0x60);
    return 1;
}

static int read_controller(uint8_t *value) {
    if (!(inb(0x64) & 1) || (inb(0x64) & 0x20)) return 0;
    *value = inb(0x60);
    return 1;
}

static void send_aux(uint8_t value) {
    wait_controller();
    outb(0x64, 0xD4);
    wait_controller();
    outb(0x60, value);
}

static int read_aux_response(uint8_t *value) {
    for (uint32_t timeout = 0; timeout < 100000; timeout++) if (read_aux(value)) return 1;
    return 0;
}

void mouse_init(void) {
    uint8_t config;
    uint8_t response;
    while (read_aux(&response)) {
    }
    wait_controller();
    outb(0x64, 0xA8);
    wait_controller();
    outb(0x64, 0x20);
    if (!read_controller(&config)) return;
    config &= (uint8_t)~0x20;
    wait_controller();
    outb(0x64, 0x60);
    wait_controller();
    outb(0x60, config);
    send_aux(0xFF);
    if (!read_aux_response(&response)) return;
    if (response == 0xFA) {
        if (!read_aux_response(&response) || response != 0xAA) return;
        read_aux_response(&response);
    }
    send_aux(0xF6);
    if (!read_aux_response(&response)) return;
    send_aux(0xF3); read_aux_response(&response);
    send_aux(200); read_aux_response(&response);
    send_aux(0xF3); read_aux_response(&response);
    send_aux(100); read_aux_response(&response);
    send_aux(0xF3); read_aux_response(&response);
    send_aux(80); read_aux_response(&response);
    send_aux(0xF2);
    if (read_aux_response(&response) && response == 0xFA && read_aux_response(&response) && response == 3) packet_length = 4;
    send_aux(0xF4);
    read_aux_response(&response);
    packet_index = 0;
}

int mouse_poll(mouse_event_t *event) {
    uint8_t value;
    if (event == 0) return 0;
    while (read_aux(&value)) {
        if (packet_index == 0 && !(value & 0x08)) continue;
        packet[packet_index++] = value;
        if (packet_index < packet_length) continue;
        packet_index = 0;
        event->buttons = packet[0] & 0x07;
        event->dx = (int8_t)packet[1];
        event->dy = (int8_t)packet[2];
        event->wheel = packet_length == 4 ? (int8_t)(packet[3] & 0x0F) : 0;
        if (event->wheel & 0x08) event->wheel |= (int8_t)0xF0;
        return 1;
    }
    return 0;
}

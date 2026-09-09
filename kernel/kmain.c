#include <stdint.h>
#include "io.h"
#include "keyboard.h"
#include "ext2.h"
#include "../bin/commands.h"

#define VGA_MEMORY 0xB8000
#define VGA_WIDTH 80
#define VGA_HEIGHT 25

uint16_t *vga_buffer = (uint16_t *)VGA_MEMORY;
uint32_t cursor_x = 0;
uint32_t cursor_y = 0;

static void vga_update_cursor(void) {
    uint16_t position = (uint16_t)(cursor_y * VGA_WIDTH + cursor_x);
    outb(0x3D4, 0x0F);
    outb(0x3D5, position & 0xFF);
    outb(0x3D4, 0x0E);
    outb(0x3D5, position >> 8);
}

extern ext2_filesystem_t fs;
extern int ramfs_init(void);

void vga_scroll(void) {
    for (int i = 0; i < (VGA_HEIGHT - 1) * VGA_WIDTH; i++) {
        vga_buffer[i] = vga_buffer[i + VGA_WIDTH];
    }
    for (int i = (VGA_HEIGHT - 1) * VGA_WIDTH; i < VGA_HEIGHT * VGA_WIDTH; i++) {
        vga_buffer[i] = (0x07 << 8) | ' ';
    }
}

void vga_putc(char c) {
    if (c == '\n') {
        cursor_x = 0;
        cursor_y++;
        if (cursor_y >= VGA_HEIGHT) {
            vga_scroll();
            cursor_y = VGA_HEIGHT - 1;
        }
        vga_update_cursor();
        return;
    }
    
    if (c == '\b') {
        if (cursor_x > 0) {
            cursor_x--;
        } else if (cursor_y > 0) {
            cursor_y--;
            cursor_x = VGA_WIDTH - 1;
        }
        uint32_t index = cursor_y * VGA_WIDTH + cursor_x;
        vga_buffer[index] = (0x07 << 8) | ' ';
        vga_update_cursor();
        return;
    }
    
    if (c == '\r') {
        cursor_x = 0;
        vga_update_cursor();
        return;
    }
    
    if (cursor_x >= VGA_WIDTH) {
        cursor_x = 0;
        cursor_y++;
        if (cursor_y >= VGA_HEIGHT) {
            vga_scroll();
            cursor_y = VGA_HEIGHT - 1;
        }
    }
    
    uint32_t index = cursor_y * VGA_WIDTH + cursor_x;
    vga_buffer[index] = (0x07 << 8) | (unsigned char)c;
    cursor_x++;
    vga_update_cursor();
}

void vga_puts(const char *str) {
    for (int i = 0; str[i]; i++) {
        vga_putc(str[i]);
    }
}

void vga_clear(void) {
    for (int i = 0; i < VGA_WIDTH * VGA_HEIGHT; i++) {
        vga_buffer[i] = (0x07 << 8) | ' ';
    }
    cursor_x = 0;
    cursor_y = 0;
    vga_update_cursor();
}

void vga_backspace(void) {
    if (cursor_x > 0) {
        cursor_x--;
    } else if (cursor_y > 0) {
        cursor_y--;
        cursor_x = VGA_WIDTH - 1;
    }
    uint32_t index = cursor_y * VGA_WIDTH + cursor_x;
    vga_buffer[index] = (0x07 << 8) | ' ';
    vga_update_cursor();
}

void vga_cursor_left(void) {
    if (cursor_x > 0) cursor_x--;
    vga_update_cursor();
}

void vga_cursor_right(void) {
    if (cursor_x < VGA_WIDTH - 1) cursor_x++;
    vga_update_cursor();
}

int strcmp(const char *a, const char *b) {
    while (*a && *b && *a == *b) {
        a++;
        b++;
    }
    return *a - *b;
}

int strlen(const char *s) {
    int i = 0;
    while (s[i]) i++;
    return i;
}

void strcpy(char *dest, const char *src) {
    int i = 0;
    while (src[i]) {
        dest[i] = src[i];
        i++;
    }
    dest[i] = '\0';
}

static void redraw_input_line(char *buffer, int length, int cursor, int old_length, int old_cursor) {
    int i;
    int drawn_length = length > old_length ? length : old_length;
    for (i = 0; i < old_cursor; i++) vga_cursor_left();
    for (i = 0; i < length; i++) vga_putc(buffer[i]);
    for (; i < old_length; i++) vga_putc(' ');
    for (i = drawn_length; i > cursor; i--) vga_cursor_left();
}

command_t* find_command(const char *name) {
    for (int i = 0; commands[i].name != 0; i++) {
        if (strcmp(commands[i].name, name) == 0) {
            return &commands[i];
        }
    }
    return 0;
}

void shell(void) {
    char buffer[80];
    char cmd_name[80];
    char param[80];
    char history[16][80];
    int history_count = 0;
    int history_pos;
    int pos;
    int length;
    
    while (1) {
        vga_puts("piux");
        if (ext2_is_mounted()) {
            vga_puts(" ");
            vga_puts(fs.current_path);
        }
        vga_puts("> ");
        pos = 0;
        length = 0;
        history_pos = history_count;
        
        while (1) {
            int c = keyboard_read_char();
            
            if (c == '\n') {
                vga_putc('\n');
                buffer[length] = '\0';
                break;
            }

            if (c == KEY_ARROW_LEFT && pos > 0) {
                pos--;
                vga_cursor_left();
                continue;
            }

            if (c == KEY_ARROW_RIGHT && pos < length) {
                pos++;
                vga_cursor_right();
                continue;
            }

            if (c == KEY_ARROW_UP && history_pos > 0) {
                int old_length = length;
                history_pos--;
                strcpy(buffer, history[history_pos]);
                length = strlen(buffer);
                pos = length;
                redraw_input_line(buffer, length, pos, old_length, old_length);
                continue;
            }

            if (c == KEY_ARROW_DOWN && history_pos < history_count) {
                int old_length = length;
                history_pos++;
                if (history_pos < history_count) strcpy(buffer, history[history_pos]);
                else buffer[0] = '\0';
                length = strlen(buffer);
                pos = length;
                redraw_input_line(buffer, length, pos, old_length, old_length);
                continue;
            }

            if (c == '\b' && pos > 0) {
                for (int i = pos - 1; i < length; i++) buffer[i] = buffer[i + 1];
                pos--;
                length--;
                redraw_input_line(buffer, length, pos, length + 1, pos + 1);
                continue;
            }

            if (c == KEY_DELETE && pos < length) {
                for (int i = pos; i < length; i++) buffer[i] = buffer[i + 1];
                length--;
                redraw_input_line(buffer, length, pos, length + 1, pos);
                continue;
            }

            if (c >= 32 && c < 127 && length < 79) {
                for (int i = length; i > pos; i--) buffer[i] = buffer[i - 1];
                buffer[pos++] = (char)c;
                length++;
                redraw_input_line(buffer, length, pos, length - 1, pos - 1);
            }
        }
        
        if (buffer[0] == '\0') {
            continue;
        }

        if (history_count < 16) {
            strcpy(history[history_count++], buffer);
        } else {
            for (int i = 1; i < 16; i++) strcpy(history[i - 1], history[i]);
            strcpy(history[15], buffer);
        }
        
        int space_pos = -1;
        for (int i = 0; buffer[i]; i++) {
            if (buffer[i] == ' ') {
                space_pos = i;
                break;
            }
        }
        
        if (space_pos > 0) {
            for (int i = 0; i < space_pos; i++) {
                cmd_name[i] = buffer[i];
            }
            cmd_name[space_pos] = '\0';
            
            int j = 0;
            int start = space_pos + 1;
            while (buffer[start + j]) {
                param[j] = buffer[start + j];
                j++;
            }
            param[j] = '\0';
        } else {
            strcpy(cmd_name, buffer);
            param[0] = '\0';
        }
        
        command_t *cmd = find_command(cmd_name);
        
        if (cmd) {
            cmd->handler(param, vga_puts, vga_putc);
        } else {
            vga_puts("Unknown command: ");
            vga_puts(cmd_name);
            vga_putc('\n');
        }
    }
}

void kernel_main(uint32_t magic, uint32_t addr) {
    vga_clear();
    vga_update_cursor();
    vga_puts("Welcome to Piux!\n");
    vga_puts("Type 'help' for essential commands explaination use.\n\n");
    
    ramfs_init();
    ext2_mount();
    
    shell();
}
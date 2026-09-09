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
        return;
    }
    
    if (c == '\r') {
        cursor_x = 0;
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
    int pos = 0;
    
    while (1) {
        vga_puts("piux> ");
        pos = 0;
        
        while (1) {
            char c = keyboard_read_char();
            
            if (c == '\n') {
                vga_putc('\n');
                buffer[pos] = '\0';
                break;
            }
            
            if (c == '\b' && pos > 0) {
                pos--;
                vga_backspace();
                continue;
            }
            
            if (c >= 32 && c < 127 && pos < 79) {
                buffer[pos++] = c;
                vga_putc(c);
            }
        }
        
        if (buffer[0] == '\0') {
            continue;
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
    vga_puts("Welcome to Piux!\n");
    vga_puts("Type 'help' for essential commands explaination use.\n\n");
    
    ramfs_init();
    ext2_mount();
    
    shell();
}
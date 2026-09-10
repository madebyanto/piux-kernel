#include <stdint.h>
#include "../kernel/io.h"
#include "../kernel/ext2.h"
#include "../tui/wm/wm.h"

typedef void (*vga_puts_t)(const char*);
typedef void (*vga_putc_t)(char);

extern int ext2_find_inode_by_path(const char *path);
extern int ext2_read_file_by_path(const char *path, char *buffer, uint32_t size);
extern int ext2_write_file_by_path(const char *path, const char *buffer, uint32_t size);
extern int keyboard_read_char(void);
extern int ramfs_write_file(const char *filename, const char *data, uint32_t size);
extern int ramfs_find_file(const char *filename);
extern int ramfs_create_file(const char *filename);
extern int ramfs_read_file(const char *filename, char *buffer, uint32_t max_size);

#define BUFFER_SIZE 8191
#define MAX_FILENAME_LEN 256

static void redraw_editor(vga_puts_t vga_puts, vga_putc_t vga_putc,
                          const char *filename, const char *buffer, int length) {
    if (!pwm_command_is_active()) return;
    pwm_command_clear();
    vga_puts("=== nano: ");
    vga_puts(filename);
    vga_puts(" ===\nCTRL+S save | CTRL+X exit | BACKSPACE delete\n---\n");
    for (int i = 0; i < length; i++) vga_putc(buffer[i]);
}

void cmd_nano(const char *filename, vga_puts_t vga_puts, vga_putc_t vga_putc) {
    if (*filename == '\0') {
        vga_puts("Usage: nano <filename>\n");
        return;
    }
    
    uint32_t fname_len = 0;
    while (filename[fname_len] != '\0' && fname_len < MAX_FILENAME_LEN - 1) {
        fname_len++;
    }
    
    char safe_filename[MAX_FILENAME_LEN];
    memset_safe(safe_filename, 0, sizeof(safe_filename));
    for (uint32_t i = 0; i < fname_len && i < MAX_FILENAME_LEN - 1; i++) {
        safe_filename[i] = filename[i];
    }
    
    char buffer[BUFFER_SIZE + 1];
    memset_safe(buffer, 0, sizeof(buffer));
    int pos = 0;
    int running = 1;
    
    int use_ext2 = ext2_is_mounted();
    int idx = use_ext2 ? ext2_find_inode_by_path(safe_filename) : ramfs_find_file(safe_filename);
    if (idx >= 0) {
        int loaded = use_ext2 ? ext2_read_file_by_path(safe_filename, buffer, BUFFER_SIZE) :
                                ramfs_read_file(safe_filename, buffer, BUFFER_SIZE);
        if (loaded > 0) {
            pos = loaded;
        }
    }
    if (pwm_command_is_active()) redraw_editor(vga_puts, vga_putc, safe_filename, buffer, pos);
    else {
        vga_puts("=== nano: ");
        vga_puts(safe_filename);
        vga_puts(" ===\nCTRL+S save | CTRL+X exit | BACKSPACE delete\n---\n");
        for (int i = 0; i < pos; i++) vga_putc(buffer[i]);
        vga_puts(pos ? "\n[Loaded]\n" : "[New file]\n");
    }
    
    while (running) {
        char c = (char)pwm_command_read_char();
        
        if (c == 19) {
            vga_puts("\n[Saving] ");

            int written;
            if (use_ext2) {
                written = ext2_write_file_by_path(safe_filename, buffer, (uint32_t)pos);
            } else {
                idx = ramfs_find_file(safe_filename);
                if (idx < 0 && ramfs_create_file(safe_filename) < 0) {
                    vga_puts("CREATE_FAILED\n");
                    continue;
                }
                written = ramfs_write_file(safe_filename, buffer, (uint32_t)pos);
            }

            if (written >= 0) {
                vga_puts("OK\n");
            } else {
                vga_puts("FAILED\n");
            }
            if (pwm_command_is_active()) redraw_editor(vga_puts, vga_putc, safe_filename, buffer, pos);
            continue;
        }
        
        if (c == 24) {
            vga_puts("\n[Exit]\n");
            running = 0;
            break;
        }
        
        if (c == 8 || c == 127) {
            if (pos > 0) {
                pos--;
                buffer[pos] = '\0';
                if (pwm_command_is_active()) redraw_editor(vga_puts, vga_putc, safe_filename, buffer, pos);
                else vga_puts("\b \b");
            }
            continue;
        }
        
        if ((c == '\n' || c == '\r') && pos < BUFFER_SIZE) {
            buffer[pos++] = '\n';
            buffer[pos] = '\0';
            if (pwm_command_is_active()) redraw_editor(vga_puts, vga_putc, safe_filename, buffer, pos);
            else vga_putc('\n');
            continue;
        }
        
        if (c >= 32 && c < 127 && pos < BUFFER_SIZE) {
            buffer[pos++] = c;
            buffer[pos] = '\0';
            if (pwm_command_is_active()) redraw_editor(vga_puts, vga_putc, safe_filename, buffer, pos);
            else vga_putc(c);
        }
    }
}

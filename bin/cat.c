#include <stdint.h>
#include "../kernel/ext2.h"

typedef void (*vga_puts_t)(const char*);
typedef void (*vga_putc_t)(char);

extern int ramfs_read_file(const char *filename, char *buffer, uint32_t size);

void cmd_cat(const char *filename, vga_puts_t vga_puts, vga_putc_t vga_putc) {
    if (*filename == '\0') {
        vga_puts("Usage: cat <filename>\n");
        return;
    }
    
    char buffer[8192];
    int bytes = ext2_is_mounted() ? ext2_read_file_by_path(filename, buffer, 8192) :
                                    ramfs_read_file(filename, buffer, 8192);
    
    if (bytes < 0) {
        vga_puts("File not found\n");
        return;
    }
    
    for (int i = 0; i < bytes; i++) {
        vga_putc(buffer[i]);
    }
    if (bytes > 0) vga_putc('\n');
}

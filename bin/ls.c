#include <stdint.h>
#include "../kernel/ext2.h"

typedef void (*vga_puts_t)(const char*);
typedef void (*vga_putc_t)(char);

extern void ramfs_list_files(void (*vga_puts)(const char*));
extern ext2_filesystem_t fs;

void cmd_ls(const char *param, vga_puts_t vga_puts, vga_putc_t vga_putc) {
    vga_puts("Files in system:\n");
    if (ext2_is_mounted()) ext2_print_dir_contents(fs.current_dir_inode, vga_puts);
    else ramfs_list_files(vga_puts);
}
